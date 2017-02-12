#ifndef WEB_ALBUM_CREATOR_HELPER_H_SENTRY
#define WEB_ALBUM_CREATOR_HELPER_H_SENTRY

char *make_user_path(char *, char *);
char *make_r_path_to_webpage(char *, char *);
char *make_path_to_css(char *);


struct WebAlbumParams;

void album_creator_debug(const WebAlbumParams &);
WebAlbumParams album_params_helper(char *, char *, char *);
void free_album_params(WebAlbumParams &);

bool create_user_paths(
        const char *user_path,
        const char *srcs_path,
        const char *thmbs_path);

// int clean_paths(const WebAlbumParams &);

#endif
