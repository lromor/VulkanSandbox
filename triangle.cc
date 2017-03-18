
#include <unistd.h>
#include <iostream>
#include <string.h>

#include <xcb/xcb.h>

#include "vulkan-core.h"

class Triangle : public VulkanCore {

public:
  Triangle() {}
  void CreateWindow(uint32_t x, uint32_t y, uint16_t width, uint16_t height);
  void Loop();
private:

  // xcb stuff
  uint16_t width_;
  uint16_t height_;
  xcb_connection_t *connection_;
  xcb_window_t window_;
  xcb_screen_t *screen_;

};


static xcb_gcontext_t getFontGC(xcb_connection_t *connection,
                          xcb_screen_t *screen,
                          xcb_window_t window,
                          const char *font_name ) {
  xcb_font_t font = xcb_generate_id(connection);
  xcb_open_font(connection, font, strlen(font_name), font_name);

  xcb_gcontext_t gc = xcb_generate_id(connection);
  uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
  uint32_t value_list[3] = {screen->black_pixel,
                            screen->white_pixel,
                            font};
  xcb_create_gc(connection, gc, window, mask, value_list );

  xcb_close_font(connection, font);

  return gc;
}


static void drawText (xcb_connection_t  *connection,
                      xcb_screen_t *screen,
                      xcb_window_t window,
                      int16_t x1,
                      int16_t y1,
                      const char *label) {
  xcb_gcontext_t gc = getFontGC (connection, screen, window, "fixed");

  xcb_void_cookie_t textCookie = \
    xcb_image_text_8_checked(connection, strlen (label), window, gc, x1, y1,
                             label);
  xcb_void_cookie_t gcCookie = xcb_free_gc (connection, gc);
}

void Triangle::Loop() {
  xcb_generic_event_t  *event;
  for (;;) {

    if ( (event = xcb_poll_for_event(connection_)) ) {
      switch (event->response_type & ~0x80) {
        case XCB_EXPOSE: {
          printf("Drawing");
          fflush(stdout);
          drawText(connection_, screen_, window_, 10, height_ - 10,
                   "Press ESC key to exit..." );
          break;
        }
        case XCB_KEY_RELEASE: {
          xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;

          switch (kr->detail) {
              // Esc
              case 9: {
                  free (event);
                  xcb_disconnect (connection_);
                  exit(0);
              }
          }
          free (event);
        }
      }

    }
  }

}

void Triangle::CreateWindow(uint32_t x, uint32_t y,
                            uint16_t width, uint16_t height) {
  // Chose the DISPLAY
  width_ = width;
  height_ = height;

  int screenp = 0;
  connection_ = xcb_connect(NULL, &screenp);

  if (xcb_connection_has_error(connection_))
    fprintf(stderr, "Problems on connectin through XCB");

  xcb_screen_iterator_t iter =
      xcb_setup_roots_iterator(xcb_get_setup(connection_));

  for (int s = screenp; s > 0; s--) xcb_screen_next(&iter);

  screen_ = iter.data;
  window_ = xcb_generate_id(connection_);

  uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t value_list[2];
  value_list[0] = screen_->black_pixel;
  value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
              XCB_EVENT_MASK_BUTTON_PRESS |
              XCB_EVENT_MASK_EXPOSURE |
              XCB_EVENT_MASK_POINTER_MOTION;

  xcb_create_window(connection_,                    // Connection
                    XCB_COPY_FROM_PARENT,           // depth (same as root)
                    window_,                        // window Id
                    screen_->root,                  // parent window
                    0, 0,                           // x, y
                    width, height,                  // width, height
                    0,                              // border_width
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,  // class
                    screen_->root_visual,           // visual
                    event_mask, value_list);        // masks, not used yet

  xcb_map_window(connection_, window_);

  const uint32_t coords[] = {x, y};
  xcb_configure_window(connection_, window_,
                       XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
  xcb_flush(connection_);
}

int main (int argc, char *argv[]) {
  Triangle a = Triangle();
  a.CreateWindow(300, 200, 800, 600);
  a.Loop();
  return 0;
}
