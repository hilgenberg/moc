#pragma once
#include "interface.h"
#include "../playlist.h"
#include "keys.h"
#include "../protocol.h"
#include "../Socket.h"

class Client
{
public:
	Client(int sock, stringlist &args);
	~Client();
	void run ();

	plist playlist, dir_plist;
	str   cwd; // current position of dir_plist
	bool  synced; // is our playlist synced with the server's?

	void handle_command(key_cmd cmd); // does everything that's not purely UI

	static volatile int  want_quit; // 1=quit client, 2=quit server
	static volatile bool want_interrupt;
	static volatile bool want_resize;

	void seek (int sec) { srv.send(CMD_SEEK); srv.send(sec); }
	void jump_to (int sec) { srv.send(CMD_JUMP_TO); srv.send(sec); }
	void seek_to_percent (int percent) { srv.send(CMD_JUMP_TO); srv.send(-percent); }

private:
	Socket srv;
	std::unique_ptr<Interface> iface;

	bool want_plist_update, want_state_update;
	
	int    silent_seek_pos = -1; /* Silent seeking - where we are in seconds. -1 - no seeking. */
	double silent_seek_key_last; /* when the silent seek key was last used */

	void wait_for_data();
	void handle_server_event(int type);
	int  get_data_int () { wait_for_data(); return srv.get_int (); }
	bool get_data_bool() { wait_for_data(); return srv.get_bool(); }
	str  get_data_str () { wait_for_data(); return srv.get_str (); }
	void send_tags_request (const str &file);

	int get_mixer_value();
	int get_channels();
	int get_rate();
	int get_bitrate();
	int get_avg_bitrate();
	int get_curr_time();
	PlayState get_state();
	
	void set_cwd(const str &path);
	void ask_for_tags (const plist &plist);
	void update_state ();
	void forward_playlist ();
	bool go_to_dir (const char *dir);
	bool go_to_playlist (const str &file);
	void set_mixer (int val);
	void adjust_mixer (int diff);
	void add_to_plist (bool recursive);
	void set_rating (int r);
	void delete_item ();
	void go_to_playing_file ();
	void seek_silent (int dt);
	void move_item (int direction);
};

inline int user_wants_interrupt ()
{
	return Client::want_interrupt;
}
