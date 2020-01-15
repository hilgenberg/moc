#pragma once

void files_init ();
void files_cleanup ();

str  add_path(const str &p1, const str &p2); // like (cd p1; cd p2; return pwd) p1,p2 assumed to be normalized!
str& normalize_path(str &p); // modifies and returns p
str  normalized_path(const str &p);
str  absolute_path(const str &p);

const char *ext_pos (const char *file);
char *ext_pos (char *file);

bool file_exists (const str &file);
bool is_dir (const str &file);
bool is_sound_file (const str &name); // in decoder.cc
bool is_url (const str &str);
bool is_plist_file (const str &name);

str containing_directory(const str &path);

time_t get_mtime (const char *file);
bool can_read_file (const char *file);
bool is_secure (const char *file);
char *file_mime_type (const char *file);

char *read_line (FILE *file);
char *find_match_dir (char *dir);

bool purge_directory (const char *dir_path);
