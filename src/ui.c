#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <libpict.h>
#include "mlinstall.h"

struct State {
    int x;
    int tab_selected;
    char log_buffer[1000];
    int log_buffer_size;
};

void usb_tab(struct State *state) {
//    if (im_begin_vertical_box()) {
        im_button("Connect to Camera");

        im_begin_disabled();
        im_label("Blah blah");
        im_button("Make the camera bootable");
        im_button("Make the camera NOT bootable");
        im_end_disabled();

//        im_end_vertical_box();
//    }
}

void tabs(struct State *state) {
    im_set_next_expand();
    if (im_begin_tab_bar(&state->tab_selected)) {
        im_set_next_inner_padding(1);
        im_set_next_gap(1);
        if (im_begin_tab("USB")) {
            usb_tab(state);
            im_end_tab();
        }
        if (im_begin_tab("Card")) {
            im_end_tab();
        }
        if (im_begin_tab("About")) {
            im_end_tab();
        }
        im_end_tab_bar();
    }
}

int mlinstall_rim_window(struct RimContext *ctx, void *arg) {
    struct State state = {0};
    while (rim_poll(ctx)) {
        im_set_next_inner_padding(1);
        if (im_begin_window("MLinstall Rim v0.2.0", 800, 500)) {
            im_set_next_expand();
            im_set_next_gap(1);
            if (im_begin_horizontal_box()) {
                im_set_next_expand();
                im_set_next_gap(1);
                if (im_begin_vertical_box()) {
                    im_label("MLinstall");
                    im_label("Tool to help install Magic Lantern");
                    tabs(&state);
                    im_end_vertical_box();
                }
                im_set_next_expand();
                if (im_begin_vertical_box()) {
                    im_set_next_expand();
                    im_multiline_entry(state.log_buffer, sizeof(state.log_buffer));
                    im_end_vertical_box();
                }
                im_end_horizontal_box();
            }
            im_end_window();
        }
    }
    return 0;
}
