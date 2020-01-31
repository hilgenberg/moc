#pragma once
#include "server.h"
# include <db.h>

// TODO: handle load() failing!

struct cache_record
{
	time_t mod_time; // last modification time of the file
	file_tags tags;
};

class tags_cache
{
public:
	tags_cache();
	~tags_cache();

	/* Request queue manipulation functions: */
	void clear_queue (int client_id);

	/* Cache DB manipulation functions: */
	void load (const str &cache_dir);
	void add_request (const char *file, int client_id, tag_changes *tags=NULL);
	file_tags get_immediate (const char *file);
	void ratings_changed(const str &file, int rating);

	void files_rm(std::set<str> &src); // unlinks all files in src, removing those that fail
	void files_mv(std::set<str> &src, const str &dst); // move file to new directory
	bool files_mv(const str &src, str &dst); // rename/move single file

private:
	DB_ENV *db_env;
	DB *db;
	u_int32_t locker;

	struct Lock
	{
		Lock(tags_cache &c, DBT &key);
		Lock(const Lock &) = delete;
		~Lock();
		DB_LOCK lock;
		DB_ENV *db_env;
	};
	friend struct Lock;

	void remove_rec(const char *fname);
	void sync();
	void add(DBT &key, const cache_record &rec);
	file_tags read_add(const char *file, int client_id);
	void write_add(const char *file, tag_changes *tags, int client_id);
	static void *reader_thread (void *cache_ptr);

	struct Request
	{
		str path;
		std::unique_ptr<tag_changes> tags;
		Request(const str &p) : path(p) {}
		Request(const str &p, tag_changes *t) : path(p), tags(t) {}
	};
	typedef std::queue<Request> request_queue;
	request_queue queues[CLIENTS_MAX]; /* requests queues for each client */
	bool stop_reader_thread; /* request for stopping read thread (if non-zero) */
	pthread_cond_t request_cond; /* condition for signalizing new requests */
	pthread_mutex_t mutex; /* mutex for all above data (except db because it's thread-safe) */
	pthread_t reader_thread_id; /* tid of the reading thread */
};
