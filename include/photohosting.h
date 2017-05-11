#ifndef PHOTOHOSTING_H_SENTRY
#define PHOTOHOSTING_H_SENTRY


struct WebAlbumParams;


class Photohosting
{
    char *path_to_static;
    char *relative_path_to_css; //relative to the path_to_static

    void _CreateAlbum(const WebAlbumParams &cfg);

public:
    Photohosting(const char *_path_to_static, const char *_relative_path_to_css);
    ~Photohosting();

    char *CreateAlbum(const char *user, const char *archive, const char *title);
};


#endif
