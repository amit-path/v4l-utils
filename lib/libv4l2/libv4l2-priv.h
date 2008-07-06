/*
#             (C) 2008 Hans de Goede <j.w.r.degoede@hhs.nl>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __LIBV4L2_PRIV_H
#define __LIBV4L2_PRIV_H

#include <stdio.h>
#include <pthread.h>
#include <libv4lconvert.h> /* includes videodev2.h for us */

/* On 32 bits archs we always use mmap2, on 64 bits archs there is no mmap2 */
#ifdef __NR_mmap2
#define SYS_mmap2 __NR_mmap2
#define MMAP2_PAGE_SHIFT 12
#else
#define SYS_mmap2 SYS_mmap
#define MMAP2_PAGE_SHIFT 0
#endif

#define V4L2_MAX_DEVICES 16
/* Warning when making this larger the frame_queued and frame_mapped members of
   the v4l2_dev_info struct can no longer be a bitfield, so the code needs to
   be adjusted! */
#define V4L2_MAX_NO_FRAMES 32
#define V4L2_DEFAULT_NREADBUFFERS 4
#define V4L2_FRAME_BUF_SIZE (4096 * 4096)

#define V4L2_LOG_ERR(...) \
  do { \
    if (v4l2_log_file) { \
      fprintf(v4l2_log_file, "libv4l2: error " __VA_ARGS__); \
      fflush(v4l2_log_file); \
    } else \
      fprintf(stderr, "libv4l2: error " __VA_ARGS__); \
  } while(0)

#define V4L2_LOG_WARN(...) \
  do { \
    if (v4l2_log_file) { \
      fprintf(v4l2_log_file, "libv4l2: warning " __VA_ARGS__); \
      fflush(v4l2_log_file); \
    } else \
      fprintf(stderr, "libv4l2: warning " __VA_ARGS__); \
  } while(0)

#define V4L2_LOG(...) \
  do { \
    if (v4l2_log_file) { \
      fprintf(v4l2_log_file, "libv4l2: " __VA_ARGS__); \
      fflush(v4l2_log_file); \
    } \
  } while(0)

#define MIN(a,b) (((a)<(b))?(a):(b))

enum v4l2_io { v4l2_io_none, v4l2_io_read, v4l2_io_mmap };

struct v4l2_dev_info {
  int fd;
  int flags;
  int open_count;
  /* actually format of the cam */
  struct v4l2_format src_fmt;
  /* fmt as seen by the application (iow after conversion) */
  struct v4l2_format dest_fmt;
  pthread_mutex_t stream_lock;
  unsigned int no_frames;
  unsigned int nreadbuffers;
  enum v4l2_io io;
  struct v4lconvert_data *convert;
  unsigned char *convert_mmap_buf;
  /* Frame bookkeeping is only done when in read or mmap-conversion mode */
  unsigned char *frame_pointers[V4L2_MAX_NO_FRAMES];
  int frame_sizes[V4L2_MAX_NO_FRAMES];
  int frame_queued; /* 1 status bit per frame */
  /* mapping tracking of our fake (converting mmap) frame buffers, todo this
     perfect we should use a map counter per frame, this is a good
     approximation but there are scenarios thinkable where this doesn't work.
     However no normal application not even a buggy one is likely to exhibit
     the patterns needed to fail this somewhat simplified tracking */
  int frame_mapped; /* 1 status bit per frame */
  int frame_map_count; /* total number of maps of (fake) buffers combined */
};

/* From log.c */
void v4l2_log_ioctl(unsigned long int request, void *arg, int result);

#endif
