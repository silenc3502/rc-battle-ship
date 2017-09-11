#ifndef LOOPBACK_H
#define LOOPBACK_H

#ifdef __cplusplus
extern "C" {
#endif

struct control {
    unsigned int main_cam;
    unsigned int num_cams;
    unsigned int num_jpeg;
    unsigned int display_xres, display_yres;
    bool pip;
    bool jpeg;
    bool exit;
};

extern struct control status;

int init_loopback(void);
void process_frame(void);
void end_streaming(void);
void exit_devices(void);
void drm_disable_pip(void);
void drm_enable_pip(void);
void set_plane_properties(void);

#ifdef __cplusplus
}
#endif

#endif // LOOPBACK_H
