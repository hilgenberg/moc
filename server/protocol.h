#pragma once

/* Definition of events sent by server to the client. */
enum ServerEvents : int
{
	EV_PONG = 1,	/* response for CMD_PING */
	EV_BUSY,	/* too many clients are connected to the server */
	EV_EXIT,	/* the server is about to exit */
	
	EV_SRV_ERROR,	/* an error occurred */
	EV_STATUS_MSG,	/* (5) followed by a status message */
	
	EV_DATA,	/* data in response to a request follows */
	EV_FILE_TAGS,	/* tags in a response for tags request */
	EV_FILE_RATING,	/* ratings changed for a file */
	
	EV_PLIST_NEW,	/* (9) replaced the playlist (no data. use CMD_PLIST_GET) */
	EV_PLIST_ADD,	/* items were added, followed by the file names and "" */
	EV_PLIST_DEL,	/* an item was deleted, followed by the index */
	EV_PLIST_MOVE,	/* an item moved, followed by its old and new indices */

	EV_STATE, 	/* (13) server has changed the play/pause/stopped state */
	EV_CTIME,	/* current time of the song has changed */
	EV_BITRATE,	/* the bitrate has changed */
	EV_RATE,	/* the rate has changed */
	EV_CHANNELS,	/* the number of channels has changed */
	EV_OPTIONS,	/* the options (repeat, shuffle, autonext) have changed */
	EV_AVG_BITRATE,	/* average bitrate has changed */
	EV_MIXER_CHANGE	/* (20) the mixer channel was changed */
};

/* Definition of server commands. */
enum ServerCommands : int
{
	CMD_PING = 1001,	/* request for EV_PONG */
	CMD_QUIT,		/* shutdown the server */
	CMD_DISCONNECT,		/* disconnect from the server */

	CMD_PLAY,		/* play i'th item on the (optionally following) playlist, or if -1, play the following path */
	CMD_STOP,		/* stop playing */
	CMD_PAUSE,		/* pause */
	CMD_UNPAUSE,		/* unpause */
	CMD_NEXT,		/* start playing next song if available */
	CMD_PREV,		/* start playing previous song if available */
	CMD_SEEK,		/* (10) seek in the current stream */
	CMD_JUMP_TO,		/* jumps to a some position in the current stream */

	CMD_PLIST_GET,		/* send the entire playlist to client */
	CMD_PLIST_ADD,		/* add following items to the playlist */
	CMD_PLIST_DEL,		/* delete an item from the server's playlist */
	CMD_PLIST_MOVE,		/* move an item */
	CMD_GET_FILE_TAGS,	/* get tags for the specified file */
	CMD_SET_FILE_TAGS,	/* update tags for the specified file */
	CMD_SET_RATING,		/* change rating for a file */

	CMD_GET_CURRENT,	/* (19) get the current song index and path */
	CMD_GET_CTIME,		/* get the current song time */
	CMD_GET_STATE,		/* get the state */
	CMD_GET_BITRATE,	/* get the bitrate */
	CMD_GET_RATE,		/* get the rate */
	CMD_GET_CHANNELS,	/* get the number of channels */
	CMD_GET_MIXER,		/* get the volume level */
	CMD_SET_MIXER,		/* set the volume level */
	CMD_GET_AVG_BITRATE,	/* get the average bitrate */
	CMD_GET_MIXER_CHANNEL_NAME,/* get the mixer channel's name */

	CMD_TOGGLE_MIXER_CHANNEL,/* (29) toggle the mixer channel */
	CMD_TOGGLE_SOFTMIXER,	/* toggle use of softmixer */
	CMD_TOGGLE_EQUALIZER,	/* toggle use of equalizer */
	CMD_EQUALIZER_REFRESH,	/* refresh EQ-presets */
	CMD_EQUALIZER_PREV,	/* select previous eq-preset */
	CMD_EQUALIZER_NEXT,	/* select next eq-preset */
	CMD_TOGGLE_MAKE_MONO,	/* toggle mono mixing */

	CMD_GET_OPTIONS,	/* request an EV_OPTIONS */
	CMD_SET_OPTION_SHUFFLE, CMD_SET_OPTION_REPEAT, CMD_SET_OPTION_AUTONEXT
};