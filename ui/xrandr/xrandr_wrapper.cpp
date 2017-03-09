// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "ui/xrandr/xrandr_wrapper.h"

#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

namespace installer {

namespace {

struct _crtc_t {
  RRCrtc id;
  RRMode mode;
  int x;
  int y;
  int width;
  int height;
  Rotation rotation;
  int noutput;
  RROutput* outputs;
  bool changed;
};
typedef struct _crtc_t crtc_t;

// Initialized |crtc| object with default values.
void InitCrtc(crtc_t* crtc) {
  Q_ASSERT(crtc != NULL);

  memset(crtc, 0, sizeof(crtc_t));
  crtc->rotation = RR_Rotate_0;
  crtc->outputs = NULL;
  crtc->changed = false;
}

// Release memories used by crtc->outputs.
void DestroyCrtc(crtc_t* crtc) {
  fprintf(stdout, "DestroyCrtc: %p\n", crtc);
  if (crtc && crtc->noutput > 0) {
    fprintf(stdout, "free crtc->output: %p\n", crtc->outputs);
    free(crtc->outputs);
    crtc->outputs = NULL;
    crtc->noutput = 0;
  }
}

// Destroy crtc_t objects created in GetCrtcs() method.
void DestroyCrtcs(crtc_t** crtcs, int& num_crtcs) {
  fprintf(stdout, "DestroyCrtcs(), %p, %d\n", *crtcs, num_crtcs);
  if (*crtcs == NULL) {
    fprintf(stdout, "DestroyCrtcs() crtcs is null\n");
    return;
  }

  for (int i = 0; i < num_crtcs; ++i) {
    crtc_t* crtc = (*crtcs) + i;
    DestroyCrtc(crtc);
  }
  free(*crtcs);
  *crtcs = NULL;
  num_crtcs = 0;
}

// Get |crtcs| from |resources|, |num_crtcs| holds number of crtc_t objects.
// Remember to call DestroyCrtcs() to release memories.
// Returns false on error.
bool GetCrtcs(Display* dpy, XRRScreenResources* resources,
              crtc_t** crtcs, int& num_crtcs) {
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);

  num_crtcs = resources->ncrtc;
  *crtcs = static_cast<crtc_t*>(calloc((size_t)num_crtcs, sizeof(crtc_t)));
  if (*crtcs == NULL) {
    return false;
  }

  // Initialize crtc list.
  for (int i = 0; i < resources->ncrtc; ++i) {
    crtc_t* crtc = (*crtcs) + i;
    InitCrtc(crtc);
    XRRCrtcInfo* rr_crtc_info = XRRGetCrtcInfo(dpy, resources,
                                               resources->crtcs[i]);
    if (rr_crtc_info == NULL) {
      // Release resources.
      DestroyCrtcs(crtcs, num_crtcs);

      fprintf(stderr, "XRRGetCrtcInfo() returns NULL\n");
      return false;
    }
    crtc->id = resources->crtcs[i];
    crtc->rotation = rr_crtc_info->rotation;
    crtc->x = rr_crtc_info->x;
    crtc->y = rr_crtc_info->y;
    crtc->width = rr_crtc_info->width;
    crtc->height = rr_crtc_info->height;
    crtc->mode = rr_crtc_info->mode;

    crtc->noutput = rr_crtc_info->noutput;

    if (crtc->noutput > 0) {
      crtc->outputs = static_cast<RROutput*>(calloc((size_t)crtc->noutput,
                                                        sizeof(RROutput)));
      if (crtc->outputs == NULL) {
        // Release resources.
        DestroyCrtcs(crtcs, num_crtcs);

        fprintf(stderr, "GetCrtcs() failed to alloc mem for outputs\n");
        return false;
      }

      // Copy output list.
      memcpy(crtc->outputs, rr_crtc_info->outputs, (size_t)crtc->noutput);
    }
  }

  return true;
}

// Apply changed defined in crtc list.
bool ApplyCrtcs(Display* dpy, XRRScreenResources* resources,
                crtc_t* crtcs, int num_crtcs) {
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);
  Q_ASSERT(crtcs != NULL);

  bool ok = true;
  for (int i = 0; i < num_crtcs; ++i) {
    crtc_t* crtc = crtcs + i;
    if (crtc == NULL || crtc->noutput <= 0 || !crtc->changed) {
      continue;
    }
    XGrabServer(dpy);

    Status status = XRRSetCrtcConfig(dpy, resources, crtc->id,
                                     CurrentTime, crtc->x, crtc->y,
                                     crtc->mode, crtc->rotation,
                                     crtc->outputs, crtc->noutput);
    if (status == RRSetConfigSuccess) {
      fprintf(stdout, "crtc updated\n");
    } else {
      ok = false;
      fprintf(stderr, "Failed to update crtc\n");
    }
    XUngrabServer(dpy);
  }

  return ok;
}

// A helper function to get x11 display and randr resources.
// Returns false if any of these objects are not initialized successfully.
// Remember to release |resources| and close |dpy| after using them.
bool GetRRResources(Display** dpy, int& screen, Window& root,
                    XRRScreenResources** resources) {
  fprintf(stdout, "GetRRResources()\n");
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);

  // Get default display.
  *dpy = XOpenDisplay(NULL);
  if (*dpy == NULL) {
    fprintf(stderr, "Failed to open display\n");
    return false;
  }

  // Get default screen.
  screen = DefaultScreen(*dpy);
  if (screen < 0 || screen >= ScreenCount(*dpy)) {
    XCloseDisplay(*dpy);

    fprintf(stderr, "Invalid screen: %d (display has %d)\n",
            screen, ScreenCount(*dpy));
    return false;
  }

  root = RootWindow(*dpy, screen);
  int event_base, error_base;
  int major, minor;

  // Query RandR extension.
  if (!XRRQueryExtension(*dpy, &event_base, &error_base) ||
      !XRRQueryVersion(*dpy, &major, &minor)) {
    XCloseDisplay(*dpy);

    fprintf(stderr, "RandR extension missing\n");
    return false;
  }
  fprintf(stdout, "RandR version %d.%d\n", major, minor);

  *resources = XRRGetScreenResources(*dpy, root);
  if (*resources == NULL) {
    XCloseDisplay(*dpy);

    fprintf(stderr, "Failed to get screen resources.\n");
    return false;
  }
  return true;
}

// Check number of outputs is less or equal to 1.
bool IsOnlyOneOutput(Display* dpy, XRRScreenResources* resources) {
  fprintf(stdout, "IsOnlyOneOutput()\n");
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);

  int connected_outputs = 0;
  for (int i = 0; i < resources->noutput; ++i) {
    XRROutputInfo* output_info = XRRGetOutputInfo(dpy, resources,
                                                  resources->outputs[i]);
    if (output_info && (output_info->connection == RR_Connected)) {
      connected_outputs ++;
    }
  }
  return connected_outputs <= 1;
}

bool IsMirrorMode(Display* dpy, XRRScreenResources* resources) {
  fprintf(stdout, "IsMirrorMode()\n");
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);
  // Get connected outputs.
  // Check whether geometry modes of connected outputs equals each other.

  unsigned int width = 0;
  unsigned int height = 0;
  int x = 0;
  int y = 0;
  bool mode_inited = false;

  fprintf(stdout, "noutput: %d\n", resources->noutput);
  for (int i = 0; i < resources->noutput; ++i) {
    XRROutputInfo* output_info = XRRGetOutputInfo(dpy, resources,
                                                  resources->outputs[i]);
    if (output_info == NULL) {
      fprintf(stdout, "IsMirrorMode() output_info is null\n");
      continue;
    }

    fprintf(stdout, "Output(%d): %s (%ld)",
            i, output_info->name, resources->outputs[i]);
    if (output_info->connection != RR_Connected) {
      fprintf(stdout, "disconnected\n");
      continue;
    } else {
      fprintf(stdout, "connected");
    }
    fprintf(stdout, "\n");

    XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(dpy, resources, output_info->crtc);
    if (crtc_info == NULL) {
      fprintf(stderr, "crtc_info is null\n");
      continue;
    }

    if (!mode_inited) {
      mode_inited = true;
      width = crtc_info->width;
      height = crtc_info->height;
      x = crtc_info->x;
      y = crtc_info->y;
    } else {
      if (width != crtc_info->width ||
          height != crtc_info->height ||
          x != crtc_info->x ||
          y != crtc_info->y) {
        return false;
      }
    }
  }

  return true;
}

// Get mode info with |mode_id|.
XRRModeInfo* GetModeInfobyId(Display* dpy, XRRScreenResources* resources,
                             RRMode mode_id) {
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);

  for (int i = 0; i < resources->nmode; ++i) {
    XRRModeInfo* mode_info = resources->modes + i;
    if (mode_info->id == mode_id) {
      return mode_info;
    }
  }
  return NULL;
}

// Walk through connected outputs to get best same mode supported by all the
// connected outputs.
RRMode GetBestSameRRMode(Display* dpy, XRRScreenResources* resources) {
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);

  for (int i = 0; i < resources->noutput; ++i) {
    XRROutputInfo* output_info_i = XRRGetOutputInfo(dpy, resources,
                                                    resources->outputs[i]);
    if (output_info_i == NULL || output_info_i->connection != RR_Connected) {
      // Ignores disconnected outputs.
      continue;
    }
    for (int j = i + 1; j < resources->noutput; ++j) {
      XRROutputInfo* output_info_j = XRRGetOutputInfo(dpy, resources,
                                                      resources->outputs[j]);
      if (output_info_j == NULL || output_info_j->connection != RR_Connected) {
        // Ignores disconnected outputs.
        continue;
      }
      fprintf(stdout, "connected outputs: %s, %s\n",
              output_info_i->name, output_info_j->name);

      for (int mode_index = 0; mode_index < resources->nmode; ++mode_index) {
        XRRModeInfo* mode_info = resources->modes + mode_index;
        if (mode_info == NULL) {
          continue;
        }

        for (int mode_index_i = 0; mode_index_i < output_info_i->nmode;
             ++mode_index_i) {
          RRMode mode_i = output_info_i->modes[mode_index_i];
          if (mode_info->id != mode_i) {
            continue;
          }

          for (int mode_index_j = 0; mode_index_j < output_info_j->nmode;
               ++mode_index_j) {
            RRMode mode_j = output_info_j->modes[mode_index_j];
            if (mode_info->id != mode_j) {
              continue;
            }

            fprintf(stdout, "Same mode: %ld %s, %dx%d\n",
                    mode_info->id, mode_info->name,
                    mode_info->width, mode_info->height);

            return mode_info->id;
          }
        }
      }
    }
  }

  return None;
}

// Switch to mirror mode.
// Checkout number of connected outputs before calling this method.
bool ToMirrorMode(Display* dpy, XRRScreenResources* resources,
                  crtc_t* crtcs, int num_crtcs) {
  fprintf(stdout, "ToMirrorMode()\n");
  Q_ASSERT(dpy != NULL);
  Q_ASSERT(resources != NULL);
  Q_ASSERT(crtcs != NULL);
  Q_ASSERT(num_crtcs > 0);
  // Get mode with same size of connected outputs.
  // Update crtcs.
  // Apply crtcs.

  RRMode best_mode = GetBestSameRRMode(dpy, resources);
  if (best_mode == None) {
    fprintf(stderr, "Failed to get same mode for outputs, do nothing.\n");
    return false;
  }

  XRRModeInfo* best_mode_info = GetModeInfobyId(dpy, resources, best_mode);
  if (best_mode_info == NULL) {
    fprintf(stderr, "failed to get best mode info\n");
    return false;
  }

  // Update crtcs.
  for (int i = 0; i < num_crtcs; ++i) {
    crtc_t* crtc = crtcs + i;
    if (crtc->noutput <= 0) {
      continue;
    }

    crtc->mode = best_mode;
    crtc->changed = true;
    crtc->x = 0;
    crtc->y = 0;
    crtc->width = best_mode_info->width;
    crtc->height = best_mode_info->height;
  }

  return ApplyCrtcs(dpy, resources, crtcs, num_crtcs);
}

}  // namespace

bool SwitchMode() {
  Display* dpy = NULL;
  int screen = 0;
  Window window;
  XRRScreenResources* resources = NULL;
  if (!GetRRResources(&dpy, screen, window, &resources)) {
    return false;
  }

  crtc_t* crtcs = NULL;
  int num_crtcs = 0;
  if (!GetCrtcs(dpy, resources, &crtcs, num_crtcs)) {
    // Release resources.
    XRRFreeScreenResources(resources);
    XCloseDisplay(dpy);

    fprintf(stderr, "Failed to get crtc_t objects\n");
    return false;
  }

  DestroyCrtcs(&crtcs, num_crtcs);
  XRRFreeScreenResources(resources);
  XCloseDisplay(dpy);

  return true;
}

bool SwitchToMirrorMode() {
  Display* dpy = NULL;
  int screen = 0;
  Window window;
  XRRScreenResources* resources = NULL;
  if (!GetRRResources(&dpy, screen, window, &resources)) {
    return false;
  }

  // Check number of outputs.
  if (IsOnlyOneOutput(dpy, resources)) {
    fprintf(stdout, "only one output\n");
    XRRFreeScreenResources(resources);
    XCloseDisplay(dpy);
    return true;
  }

  if (IsMirrorMode(dpy, resources)) {
    XRRFreeScreenResources(resources);
    XCloseDisplay(dpy);

    fprintf(stdout, "is mirror mode\n");
    return true;
  }

  fprintf(stdout, "switch into mirror mode\n");

  crtc_t* crtcs = NULL;
  int num_crtcs = 0;
  if (!GetCrtcs(dpy, resources, &crtcs, num_crtcs)) {
    // Release resources.
    XRRFreeScreenResources(resources);
    XCloseDisplay(dpy);

    fprintf(stderr, "Failed to get crtc_t objects\n");
    return false;
  }

  ToMirrorMode(dpy, resources, crtcs, num_crtcs);

  DestroyCrtcs(&crtcs, num_crtcs);
  XRRFreeScreenResources(resources);
  XCloseDisplay(dpy);

  return true;
}

}  // namespace installer
