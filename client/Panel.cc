/*
 * MOC - music on console
 * Copyright (C) 2002 - 2006 Damian Pietras <daper@daper.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include "Panel.h"
#include "../rcc.h"
#include "utf8.h"
#include "../server/input/decoder.h"
#include "themes.h"
#include "interface.h"
#include "client.h"

// distribute available screen width among columns
static void distribute(int W, int &c0, int &c1, int &c2)
{
	assert(W >= 0 && c0 >= 0 && c1 >= 0 && c2 >= 0);
	if (c0+c1+c2 <= W) return;

	// start with even split, then reclaim wasted space
	int w0 = W/3, w1 = w0, w2 = W-w0-w1;
	int o0 = c0-w0, o1 = c1-w1, o2 = c2-w2;
	if (o0 < 0 || o1 < 0 || o2 < 0)
	{
		int w = W;
		assert(o0 >= 0 || o1 >= 0 || o2 >= 0); // otherwise we would have returned already
		if (o0 < 0) { w0 = c0; w -= c0; }
		if (o1 < 0) { w1 = c1; w -= c1; }
		if (o2 < 0) { w2 = c2; w -= c2; }
		if      (o0 < 0 && o1 < 0) { w2 = w; }
		else if (o0 < 0 && o2 < 0) { w1 = w; }
		else if (o1 < 0 && o2 < 0) { w0 = w; }
		else if (o0 < 0)
		{
			w1 = w/2, w2 = w-w1;
			o1 = c1-w1; o2 = c2-w2;
			if      (o1 < 0) { w1 = c1; w2 = w-c1; }
			else if (o2 < 0) { w2 = c2; w1 = w-c2; }
		}
		else if (o1 < 0)
		{
			w0 = w/2, w2 = w-w0;
			o0 = c0-w0; o2 = c2-w2;
			if      (o0 < 0) { w0 = c0; w2 = w-c0; }
			else if (o2 < 0) { w2 = c2; w0 = w-c2; }
		}
		else if (o2 < 0)
		{
			w0 = w/2, w1 = w-w0;
			o0 = c0-w0; o1 = c1-w1;
			if      (o0 < 0) { w0 = c0; w1 = w-c0; }
			else if (o1 < 0) { w1 = c1; w0 = w-c1; }
		}
	}
	c0 = w0; c1 = w1; c2 = w2;
	assert(c0 >= 0 && c1 >= 0 && c2 >= 0);
	assert(c0+c1+c2 == W);
}
static void distribute(int w, int &c0, int &c1)
{
	assert(w >= 0 && c0 >= 0 && c1 >= 0);
	if (c0+c1 <= w) return;

	// start with even split, then reclaim wasted space
	int w0 = w/2, w1 = w-w0;
	int o0 = c0-w0, o1 = c1-w1;
	if      (o0 < 0) { w0 = c0; w1 = w-c0; }
	else if (o1 < 0) { w1 = c1; w0 = w-c1; }
	c0 = w0; c1 = w1;
	assert(c0 >= 0 && c1 >= 0);
	assert(c0+c1 == w);
}

bool Panel::mark_path(const str &f)
{
	bool s = (sel >= 0 && sel == mark && !xsel);
	mark = items.find(f);
	if (s && mark >= 0) sel = mark;
	return mark >= 0;
}
void Panel::mark_item(int i)
{
	bool s = (sel >= 0 && sel == mark && !xsel);
	mark = i;
	if (s && mark >= 0) sel = mark;
}
bool Panel::select_path(const str &f)
{
	int i = items.find(f);
	if (i >= 0) select_item(i);
	return i >= 0;
}
void Panel::select_item(int i)
{
	if (i >= items.size()) i = items.size()-1;
	if (i == sel && xsel == 0) return;
	sel = i; xsel = 0;
	iface->redraw(2);
}

void Panel::move_selection(menu_request req)
{
	const int N = items.size();
	if (!N) return;
	if (sel < 0) xsel = 0;
	if (sel < 0 && mark >= 0 && mark < N) sel = mark;
	switch (req)
	{
		case REQ_UP:
			sel = std::min(sel, sel+xsel); xsel = 0;
			sel = (sel < 0 ? N-1 : sel - 1);
			break;
		case REQ_DOWN:
			sel = std::max(sel, sel+xsel); xsel = 0;
			sel = (sel < 0 ? 0 : sel + 1);
			break;
		case REQ_SCROLL_UP:
			sel = std::min(sel, sel+xsel); xsel = 0;
			top -= (bounds.h - 1)/2; if (top < 0) top = 0;
			sel -= (bounds.h - 1)/2;
			break;
		case REQ_SCROLL_DOWN:
			sel = std::max(sel, sel+xsel); xsel = 0;
			top += (bounds.h - 1)/2;
			sel += (bounds.h - 1)/2;
			if (top + (bounds.h-1) >= N) top = N - bounds.h;
			if (top < 0) top = 0;
			break;
		case REQ_PGUP:
			sel = std::min(sel, sel+xsel); xsel = 0;
			top -= (bounds.h - 1); if (top < 0) top = 0;
			sel -= (bounds.h - 1);
			break;
		case REQ_PGDOWN:
			sel = std::max(sel, sel+xsel); xsel = 0;
			top += (bounds.h - 1);
			sel += (bounds.h - 1);
			if (top + (bounds.h-1) >= N) top = N - bounds.h;
			if (top < 0) top = 0;
			break;
		case REQ_TOP:    sel = 0; xsel = 0; break;
		case REQ_BOTTOM: sel = N-1; xsel = 0; break;

		case REQ_XUP:  if (sel > 0) { --sel; ++xsel; } break;
		case REQ_XDOWN: if (sel >= 0 && sel+1 < N) { ++sel; --xsel; } break;
	}
	if (sel >= N) sel = N-1;
	if (sel <  0) sel = 0;
}
void Panel::handle_click(int x, int y, bool dbl)
{
	const int N = items.size();
	int i = top+y-bounds.y;
	bool hit = (i >= 0 && i < N);
	if (i >= N) i = N-1;
	if (i <  0) i = 0;
	if (i != sel || xsel) { sel = i; xsel = 0; iface->redraw(2); }
	if (dbl && hit) iface->client.handle_command(KEY_CMD_GO);
}

void Panel::draw(bool active) const
{
	auto &win = iface->win;

	const int N = items.size();
	const int lookahead = std::min(5, bounds.h/4);
	
	if (sel < 0) xsel = 0;
	if (sel < -1) sel = -1; if (mark < -1) mark = -1;
	if (sel >= N) sel = std::max(0, N-1);
	if (mark >= N) mark = -1;
	if (sel >= 0 && N > 0)
	{
		if (sel-lookahead < top) top = sel-lookahead;
		if (sel+lookahead > top + bounds.h-1) top = sel+lookahead - (bounds.h-1);
	}

	if (top + bounds.h > N) top = N-bounds.h;
	if (top < 0) top = 0;

	bool have_up = items.is_dir && N && iface->cwd() != "/";
	str mhome = options::MusicDir; if (!mhome.empty()) mhome += '/'; if (mhome.length() < 2) mhome.clear();
	str uhome = options::Home;     if (!uhome.empty()) uhome += '/'; if (uhome.length() < 2) uhome.clear();

	// gather layout data: widths for three columns (artist, album, title)
	// and maximum track number
	int extra = 5 /*rating*/ + 5 /*time*/ + 3 /*[|]*/ + 3*2 /*spacing*/;
	int c0 = 0, c1 = 0, c2 = 0, M = 0;
	bool readtags = options::ReadTags;
	int avail = bounds.w - extra;
	if (avail-2 < 3*4) readtags = false;
	bool all_same_artist = false, all_same_album = false;
	int n_tagged = 0;
	if (readtags)
	{
		bool first = true, first_tagged = true;
		str common_artist, common_album; // of all sound files (empty if any have no tags)

		for (const auto &ip : items.items)
		{
			const auto &it = *ip;
			if (first) { first = false; if (have_up) continue; } // should not have tags anyway...

			if (it.type == F_SOUND && !it.tags)
			{
				first_tagged = false;
				common_artist.clear();
				common_album.clear();
			}

			if (!it.tags) continue;
			auto &tags = *it.tags;

			if (tags.title.empty())
			{
				continue;
			}
			else
			{
				++n_tagged;
				str v0 = sanitized(tags.artist);
				str v1 = sanitized(tags.album);
				if (first_tagged)
				{
					common_artist = v0;
					common_album = v1;
					first_tagged = false;
				}
				else
				{
					if (v0 != common_artist) common_artist.clear();
					if (v1 != common_album) common_album.clear();
				}
				M  = std::max(M,  tags.track);
				c0 = std::max(c0, (int)strwidth(v0));
				c1 = std::max(c1, (int)strwidth(v1));
				c2 = std::max(c2, (int)strwidth(sanitized(tags.title)));
			}
		}
		all_same_artist = !common_artist.empty();
		all_same_album  = !common_album.empty();
	}

	int prefix_len = 0; // how much to cut off from file paths
	if (!items.is_dir)
	{
		bool first = true;
		str common_prefix; // of all paths (even those with tags!)
		for (const auto &ip : items.items)
		{
			const auto &it = *ip;
			if (it.type == F_URL) continue;
			if (first)
			{
				common_prefix = it.path;
				first = false;
			}
			else
			{
				intersect(common_prefix, it.path);
			}
		}
		prefix_len = (int)common_prefix.length();
		while (prefix_len > 0 && common_prefix[prefix_len-1] != '/') --prefix_len;
		if (prefix_len == 1) prefix_len = 0;
	}

	int cn = 0; // how wide are the playlist or track numbers?
	if (!items.is_dir)
	{
		M = N;
		for (cn = 2, M /= 10; M > 0; M /= 10) ++cn;
	}
	else if (M > 0)
	{
		for (cn = 2, M /= 10; M > 0; M /= 10) ++cn;
	}
	avail -= cn;

	if (avail < 3*4)
	{
		readtags = false;
		c0 = c1 = c2 = 0;
		if (items.is_dir) cn = 0;
		extra = 0;
		avail = bounds.w - cn;
	}
	if (avail < 6)
	{
		win.color(CLR_MENU_ITEM_FILE);
		str s(":::TOO SMALL:::");
		for (int y = 0; y < bounds.h; ++y)
		{
			int i = y-(bounds.h-1)/2 + s.length()/2;
			if (i < 0 || i >= s.length()) continue;
			win.put(bounds.y + y, bounds.x+(bounds.w-1)/2, s[i]);
		}
		return;
	}

	bool hide_artist = false, hide_album = false;
	if (readtags && c0+c1+c2 > avail)
	{
		if (all_same_artist && n_tagged > 1)
		{
			hide_artist = true;
			c0 = 0; extra -= 3; avail += 3;

			if (c1+c2 > avail)
			{
				if (all_same_album)
				{
					hide_album = true;
					c1 = 0; extra -= 3; avail += 3;
					c2 = avail;
				}
				else
				{
					distribute(avail, c1, c2);
				}
			}
		}
		else if (all_same_album) // compilation or split-CD
		{
			hide_album = true;
			c1 = 0; extra -= 3; avail += 3;
			distribute(avail, c0, c2);
		}
		else
		{
			distribute(avail, c0, c1, c2);
		}
	}
	assert(c0+c1+c2+cn+extra <= bounds.w);
	
	// draw the visible items
	for (int i = top, n = std::min(top + bounds.h, N); i < n; ++i)
	{
		bool is_up_dir = (have_up && i == 0); // is it ".." ?

		const auto &it = *items.items[i];
		int y = bounds.y + i-top, x0 = bounds.x, x1 = bounds.x + bounds.w-1;

		bool selected = active && sel >= 0 && (
			(xsel >= 0 && i >= sel && i <= sel+xsel) ||
			(xsel <  0 && i >= sel+xsel && i <= sel));

		auto info_color = 
			selected && i == mark ? CLR_MENU_ITEM_INFO_MARKED_SELECTED :
			selected ? CLR_MENU_ITEM_INFO_SELECTED :
			i == mark ? CLR_MENU_ITEM_INFO_MARKED :
			CLR_MENU_ITEM_INFO;
		auto file_color = 
			selected && i == mark ? CLR_MENU_ITEM_FILE_MARKED_SELECTED :
			i == mark ? CLR_MENU_ITEM_FILE_MARKED :
			it.type == F_DIR ? (selected ? CLR_MENU_ITEM_DIR_SELECTED : CLR_MENU_ITEM_DIR) :
			it.type == F_PLAYLIST ? (selected ? CLR_MENU_ITEM_PLAYLIST_SELECTED : CLR_MENU_ITEM_PLAYLIST) :
			selected ? CLR_MENU_ITEM_FILE_SELECTED : CLR_MENU_ITEM_FILE;
		
		win.color(info_color);

		// playlist numbering
		if (!items.is_dir && cn)
		{
			win.moveto(y, x0);
			win.put_ascii(format("%*d ", cn-1, i+1));
			x0 += cn;
		}

		// rating and time
		if (readtags && it.type != F_DIR)
		{
			x1 -= 13;
			win.put(y, x1+1, '[');
		
			win.color(file_color);
			wattroff(win.WIN(), A_BOLD); // some utf8 chars do not have bold versions
			int rating = (it.tags ? it.tags->rating : 0);
			if (rating < 0) rating = 0;
			if (rating > 5) rating = 5;

			win.put(options::rating_strings[rating]);

			win.color(info_color);
			win.put(win.vert);
			if (it.tags && it.tags->time > -1)
				win.time(it.tags->time);
			else
				win.put_ascii(it.type == F_URL ? "--:--" : "     ");
			win.put(']');
		}

		win.color(file_color);
		win.moveto(y, x0);

		if (!is_up_dir && readtags && it.tags && !it.tags->title.empty())
		{
			auto &tags = *it.tags;
			if (!hide_artist)
			{
				win.field(sanitized(tags.artist), c0);
				win.put_ascii("   ");
			}
			if (!hide_album)
			{
				win.field(sanitized(tags.album), c1);
				win.put_ascii("   ");
			}

			if (items.is_dir)
			{
				win.put_ascii(tags.track > 0 ?
					format("%*d ", cn-1, tags.track) :
					spaces(cn));
			}
			win.field(sanitized(tags.title), c2);
		}
		else if (is_up_dir && it.type == F_DIR)
		{
			win.field("../", x1-x0+1, 'l');
		}
		else if (it.type == F_URL)
		{
			win.field(sanitized(it.path), x1-x0+1, 'c');
		}
		else
		{
			str s = it.path;
			if (items.is_dir || !options::PlaylistFullPaths)
			{
				auto i = s.rfind('/', s.length()-2);
				if (i != str::npos) s = s.substr(i+1);
			}
			else if (prefix_len > 0)
			{
				s = s.substr(prefix_len);
			}
			else if (!mhome.empty() && has_prefix(s, mhome, false))
			{
				s = s.substr(mhome.length());
			}
			else if (!uhome.empty() && has_prefix(s, uhome, false))
			{
				assert(uhome.length() >= 3);
				s = s.substr(uhome.length()-2);
				s[0] = '~'; s[1] = '/';
			}

			if (it.type == F_DIR && s.back() != '/') s += '/';
			if (it.type == F_SOUND && options::HideFileExtension)
			{
				const char *file = s.c_str(), *ext = ext_pos(file);
				if (ext) s = s.substr(0, ext-1-file);
			}
			sanitize(s);
			if (options::FileNamesIconv) s = files_iconv_str (s);
			if (options::UseRCCForFilesystem) s = rcc_reencode(s);
			win.field(s, x1-x0+1, 'l');
		}
	}
}