#ifndef WEB_ALBUM_CREATOR_HELPER_H_SENTRY
#define WEB_ALBUM_CREATOR_HELPER_H_SENTRY

char *make_user_path(const char *path_to_static, const char *user);
char *make_r_path_to_webpage(const char *user, const char *random_id);
char *make_path_to_css(const char *path_to_css);


struct WebAlbumParams;

void album_creator_debug(const WebAlbumParams &);

WebAlbumParams album_params_helper(
        const char *user,
        const char *user_path,
        const char *random_id);

void free_album_params(WebAlbumParams &);

bool create_user_paths(const char *srcs_path, const char *thmbs_path);

int clean_paths(const WebAlbumParams &cfg);

#endif
