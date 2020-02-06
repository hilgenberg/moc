/*
 * MOC - music on console
 * Copyright (C) 2004 Damian Pietras <daper@daper.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include "decoder.h"
#include "io.h"
#include "inputs.h"

struct plugin
{
	plugin(const char *name, decoder *f) : name(name), decoder(f)
	{
		if (f->init) f->init();
	}
	const char *name;
	struct decoder *decoder;
};
static std::vector<plugin> plugins;

static void split_mime(str &type, str &subtype)
{
	auto i = type.find('/');
	if (i == str::npos) return;
	
	subtype = type.substr(i+1);
	type = type.substr(0, i);

	if (has_prefix(subtype, "x-", true)) subtype = subtype.substr(2);
	i = subtype.find(';');
	if (i != str::npos) subtype = subtype.substr(0, i);
}

struct decoder_preference
{
	decoder_preference(const str &s)
	{
		strings tokens = split(s, "(,)");
		assert(tokens.size() >= 1);
		type = tokens[0]; tokens.erase(tokens.begin());
		split_mime(type, subtype);

		int asterisk_at = -1;
		std::set<int> done;

		/* Add the index of each known decoder to the decoders list.
		* Note the position following the first asterisk. */
		for (auto &name : tokens)
		{
			if (name == "*")
			{
				if (asterisk_at == -1) asterisk_at = decoder_list.size();
				continue;
			}

			int d = 0; for (; d < plugins.size(); ++d) if (!strcasecmp(plugins[d].name, name.c_str())) break;
			if (d >= plugins.size() || done.count(d)) continue;
			decoder_list.push_back(d);
			done.insert(d);
		}

		if (asterisk_at == -1) return;

		for (int i = 0; i < plugins.size(); ++i)
		{
			if (done.count(i)) continue;
			decoder_list.insert(decoder_list.begin() + asterisk_at++, i);
		}
	}
	std::vector<int> decoder_list; /* decoder indices */
	str type, subtype; /* MIME type or filename extn, MIME subtype or NULL */
};
static std::vector<decoder_preference> preferences;
static std::vector<int> default_decoder_list;

/* Return the index of the first decoder able to handle files with the
 * given filename extension, or -1 if none can. */
static decoder* find_extn_decoder (const std::vector<int> &decoder_list, const char *extn)
{
	assert (extn && extn[0]);

	for (int i : decoder_list) {
		if (plugins[i].decoder->our_format_ext &&
		    plugins[i].decoder->our_format_ext (extn))
			return plugins[i].decoder;
	}

	return NULL;
}

/* Return the index of the first decoder able to handle audio with the
 * given MIME media type, or -1 if none can. */
static decoder* find_mime_decoder (const std::vector<int> &decoder_list, const str &mime)
{
	for (int i : decoder_list) {
		if (plugins[i].decoder->our_format_mime &&
		    plugins[i].decoder->our_format_mime (mime.c_str()))
			return plugins[i].decoder;
	}

	return NULL;
}

/* Return the index of the first decoder able to handle audio with the
 * given filename extension and/or MIME media type, or -1 if none can. */
static decoder* find_decoder (const char *file, str *mime)
{
	const char *extn = file ? ext_pos(file) : NULL;

	str type, subtype;
	for (auto &pref : preferences)
	{
		if (pref.subtype.empty()) {
			if (!extn || strcasecmp(pref.type.c_str(), extn)) continue;
		}
		else
		{
			if (type.empty())
			{
				if (options::UseMimeMagic && (!mime || mime->empty()) && file && *file)
				{
					type = file_mime_type(file);
					split_mime(type, subtype);
				}
				else if (mime)
				{
					type = *mime;
					split_mime(type, subtype);
				}
				else type = "/";
			}

			if (strcasecmp(pref.type.c_str(), type.c_str()) ||
			    strcasecmp(pref.subtype.c_str(), subtype.c_str())) continue;
		}

		// match
		if (!pref.subtype.empty())
			return find_mime_decoder (pref.decoder_list, *mime);
		else
			return find_extn_decoder (pref.decoder_list, extn);
	}

	decoder *d = NULL;
	if (mime && !mime->empty())
		d = find_mime_decoder (default_decoder_list, *mime);
	if (!d && extn && *extn)
		d = find_extn_decoder (default_decoder_list, extn);
	return d;
}

bool is_sound_file (const str &name)
{
	return find_decoder(name.c_str(), NULL);
}

struct decoder *get_decoder (const str &file)
{
	return find_decoder(file.c_str(), NULL);
}

/* Return the decoder for this stream. */
decoder *get_decoder_by_content (struct io_stream *stream)
{
	char buf[8096];
	ssize_t res;

	assert (stream != NULL);

	/* Peek at the start of the stream to check if sufficient data is
	 * available.  If not, there is no sense in trying the decoders as
	 * each of them would issue an error.  The data is also needed to
	 * get the MIME type. */
	logit ("Testing the stream...");
	res = io_peek (stream, buf, sizeof (buf));
	if (res < 0) {
		error("Stream error: %s", io_strerror (stream));
		return NULL;
	}

	if (res < 512) {
		logit ("Stream too short");
		return NULL;
	}

	// get decoder by mime type
	const char *tmp = io_get_mime_type (stream);
	if (tmp) {
		str mime = tmp;
		decoder *d = find_decoder (NULL, &mime);
		if (d) {
			logit ("Found decoder for MIME type %s", mime.c_str());
			return d;
		}
	}
	else logit ("No MIME type.");

	for (auto &p : plugins) {
		if (p.decoder->can_decode
				&& p.decoder->can_decode(stream)) {
			logit ("Found decoder for stream: %s", p.name);
			return p.decoder;
		}
	}

	error("Format not supported");
	return NULL;
}

void decoder_init()
{
	#define H(X) plugins.emplace_back(#X, X ## _plugin())
	ALL_INPUTS
	#undef H

	for (int ix = 0; ix < plugins.size(); ix += 1)
		default_decoder_list.push_back(ix);

	const char *PreferredDecoders[] = {
	"aac(aac,ffmpeg)", "m4a(ffmpeg)", "mpc(musepack,*,ffmpeg)", "mpc8(musepack,*,ffmpeg)",
	"sid(sidplay2)", "mus(sidplay2)", "wav(sndfile,*,ffmpeg)", "wv(wavpack,*,ffmpeg)",
	"audio/aac(aac)", "audio/aacp(aac)", "audio/m4a(ffmpeg)", "audio/wav(sndfile,*)",
	"oga(vorbis,*,ffmpeg)", "ogg(vorbis,*,ffmpeg)", "ogv(ffmpeg)", "application/ogg(vorbis)",
	"audio/ogg(vorbis)", "flac(flac,*,ffmpeg)", "opus(ffmpeg)", "spx(speex)"};
	for (auto *s : PreferredDecoders) preferences.emplace_back(s);
}

void decoder_cleanup ()
{
	for (auto &p : plugins) 
		if (p.decoder->destroy) p.decoder->destroy();

	preferences.clear();
}

/* Fill the error structure with an error of a given type and message.
 * strerror(add_errno) is appended at the end of the message if add_errno != 0.
 * The old error message is free()ed.
 * This is thread safe; use this instead of constructs using strerror(). */
void decoder_error (struct decoder_error *error,
		const enum decoder_error_type type, const int add_errno,
		const char *format, ...)
{
	char *err_str;
	va_list va;

	if (error->err)
		free (error->err);

	error->type = type;

	va_start (va, format);
	err_str = format_msg_va (format, va);
	va_end (va);

	if (add_errno) {
		char *err_buf;

		err_buf = xstrerror (add_errno);
		error->err = format_msg ("%s%s", err_str, err_buf);
		free (err_buf);
	}
	else
		error->err = format_msg ("%s", err_str);

	free (err_str);
}

/* Initialize the decoder_error structure. */
void decoder_error_init (struct decoder_error *error)
{
	error->type = ERROR_OK;
	error->err = NULL;
}

/* Set the decoder_error structure to contain "success" information. */
void decoder_error_clear (struct decoder_error *error)
{
	error->type = ERROR_OK;
	if (error->err) {
		free (error->err);
		error->err = NULL;
	}
}

void decoder_error_copy (struct decoder_error *dst,
		const struct decoder_error *src)
{
	dst->type = src->type;
	dst->err = xstrdup (src->err);
}

/* Return the error text from the decoder_error variable. */
const char *decoder_error_text (const struct decoder_error *error)
{
	return error->err;
}
