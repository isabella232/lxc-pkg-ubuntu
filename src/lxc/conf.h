/*
 * lxc: linux Container library
 *
 * (C) Copyright IBM Corp. 2007, 2008
 *
 * Authors:
 * Daniel Lezcano <daniel.lezcano at free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef __LXC_CONF_H
#define __LXC_CONF_H

#include "config.h"

#include <stdio.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/param.h>
#include <sys/types.h>
#include <stdbool.h>

#include "list.h"
#include "start.h" /* for lxc_handler */

#if HAVE_SCMP_FILTER_CTX
typedef void * scmp_filter_ctx;
#endif

/* worth moving to configure.ac? */
#define subuidfile "/etc/subuid"
#define subgidfile "/etc/subgid"

/*
 * Defines a generic struct to configure the control group.
 * It is up to the programmer to specify the right subsystem.
 * @subsystem : the targeted subsystem
 * @value     : the value to set
 */
struct lxc_cgroup {
	char *subsystem;
	char *value;
};

enum idtype {
	ID_TYPE_UID,
	ID_TYPE_GID
};

/*
 * id_map is an id map entry.  Form in confile is:
 * lxc.id_map = u 0    9800 100
 * lxc.id_map = u 1000 9900 100
 * lxc.id_map = g 0    9800 100
 * lxc.id_map = g 1000 9900 100
 * meaning the container can use uids and gids 0-99 and 1000-1099,
 * with [ug]id 0 mapping to [ug]id 9800 on the host, and [ug]id 1000 to
 * [ug]id 9900 on the host.
 */
struct id_map {
	enum idtype idtype;
	unsigned long hostid, nsid, range;
};

/*
 * Defines a structure containing a pty information for
 * virtualizing a tty
 * @name   : the path name of the slave pty side
 * @master : the file descriptor of the master
 * @slave  : the file descriptor of the slave
 */
struct lxc_pty_info {
	char name[MAXPATHLEN];
	int master;
	int slave;
	int busy;
};

/*
 * Defines the number of tty configured and contains the
 * instantiated ptys
 * @nbtty = number of configured ttys
 */
struct lxc_tty_info {
	int nbtty;
	struct lxc_pty_info *pty_info;
};

struct lxc_tty_state;

/*
 * Defines the structure to store the console information
 * @peer   : the file descriptor put/get console traffic
 * @name   : the file name of the slave pty
 */
struct lxc_console {
	int slave;
	int master;
	int peer;
	struct lxc_pty_info peerpty;
	struct lxc_epoll_descr *descr;
	char *path;
	char *log_path;
	int log_fd;
	char name[MAXPATHLEN];
	struct termios *tios;
	struct lxc_tty_state *tty_state;
};

/*
 * Defines a structure to store the rootfs location, the
 * optionals pivot_root, rootfs mount paths
 * @path       : the rootfs source (directory or device)
 * @mount      : where it is mounted
 * @options    : mount options
 * @bev_type   : optional backing store type
 */
struct lxc_rootfs {
	char *path;
	char *mount;
	char *options;
	char *bdev_type;
};

/*
 * Automatic mounts for LXC to perform inside the container
 */
enum {
	LXC_AUTO_PROC_RW              = 0x001,   /* /proc read-write */
	LXC_AUTO_PROC_MIXED           = 0x002,   /* /proc/sys and /proc/sysrq-trigger read-only */
	LXC_AUTO_PROC_MASK            = 0x003,

	LXC_AUTO_SYS_RW               = 0x004,   /* /sys */
	LXC_AUTO_SYS_RO               = 0x008,   /* /sys read-only */
	LXC_AUTO_SYS_MIXED            = 0x00C,   /* /sys read-only and /sys/class/net read-write */
	LXC_AUTO_SYS_MASK             = 0x00C,

	LXC_AUTO_CGROUP_RO            = 0x010,   /* /sys/fs/cgroup (partial mount, read-only) */
	LXC_AUTO_CGROUP_RW            = 0x020,   /* /sys/fs/cgroup (partial mount, read-write) */
	LXC_AUTO_CGROUP_MIXED         = 0x030,   /* /sys/fs/cgroup (partial mount, paths r/o, cgroup r/w) */
	LXC_AUTO_CGROUP_FULL_RO       = 0x040,   /* /sys/fs/cgroup (full mount, read-only) */
	LXC_AUTO_CGROUP_FULL_RW       = 0x050,   /* /sys/fs/cgroup (full mount, read-write) */
	LXC_AUTO_CGROUP_FULL_MIXED    = 0x060,   /* /sys/fs/cgroup (full mount, parent r/o, own r/w) */
	/* These are defined in such a way as to retain
	 * binary compatibility with earlier versions of
	 * this code. If the previous mask is applied,
	 * both of these will default back to the _MIXED
	 * variants, which is safe. */
	LXC_AUTO_CGROUP_NOSPEC        = 0x0B0,   /* /sys/fs/cgroup (partial mount, r/w or mixed, depending on caps) */
	LXC_AUTO_CGROUP_FULL_NOSPEC   = 0x0E0,   /* /sys/fs/cgroup (full mount, r/w or mixed, depending on caps) */
	LXC_AUTO_CGROUP_FORCE         = 0x100,   /* mount cgroups even when cgroup namespaces are supported */
	LXC_AUTO_CGROUP_MASK          = 0x1F0,   /* all known cgroup options, doe not contain LXC_AUTO_CGROUP_FORCE */
	LXC_AUTO_ALL_MASK             = 0x1FF,   /* all known settings */
};

/*
 * Defines the global container configuration
 * @rootfs     : root directory to run the container
 * @pivotdir   : pivotdir path, if not set default will be used
 * @mount      : list of mount points
 * @tty        : numbers of tty
 * @pts        : new pts instance
 * @mount_list : list of mount point (alternative to fstab file)
 * @network    : network configuration
 * @utsname    : container utsname
 * @fstab      : path to a fstab file format
 * @caps       : list of the capabilities to drop
 * @keepcaps   : list of the capabilities to keep
 * @tty_info   : tty data
 * @console    : console data
 * @ttydir     : directory (under /dev) in which to create console and ttys
 * @lsm_aa_profile : apparmor profile to switch to or NULL
 * @lsm_se_context : selinux type to switch to or NULL
 */
enum lxchooks {
	LXCHOOK_PRESTART,
	LXCHOOK_PREMOUNT,
	LXCHOOK_MOUNT,
	LXCHOOK_AUTODEV,
	LXCHOOK_START,
	LXCHOOK_STOP,
	LXCHOOK_POSTSTOP,
	LXCHOOK_CLONE,
	LXCHOOK_DESTROY,
	NUM_LXC_HOOKS
};

extern char *lxchook_names[NUM_LXC_HOOKS];

struct lxc_conf {
	int is_execute;
	char *fstab;
	unsigned int tty;
	unsigned int pts;
	int reboot;
	int need_utmp_watch;
	signed long personality;
	struct utsname *utsname;
	struct lxc_list cgroup;
	struct {
		struct lxc_list id_map;

		/* Pointer to the idmap entry for the container's root uid in
		 * the id_map list. Do not free! */
		struct id_map *root_nsuid_map;

		/* Pointer to the idmap entry for the container's root gid in
		 * the id_map list. Do not free! */
		struct id_map *root_nsgid_map;
	};
	struct lxc_list network;
	int auto_mounts;
	struct lxc_list mount_list;
	struct lxc_list caps;
	struct lxc_list keepcaps;
	struct lxc_tty_info tty_info;
	char *pty_names; // comma-separated list of lxc.tty pty names
	struct lxc_console console;
	struct lxc_rootfs rootfs;
	char *ttydir;
	int close_all_fds;
	struct lxc_list hooks[NUM_LXC_HOOKS];

	char *lsm_aa_profile;
	unsigned int lsm_aa_allow_incomplete;
	char *lsm_se_context;
	int tmp_umount_proc;
	char *seccomp;  // filename with the seccomp rules
#if HAVE_SCMP_FILTER_CTX
	scmp_filter_ctx seccomp_ctx;
#endif
	int maincmd_fd;
	unsigned int autodev;  // if 1, mount and fill a /dev at start
	int haltsignal; // signal used to halt container
	int rebootsignal; // signal used to reboot container
	int stopsignal; // signal used to hard stop container
	unsigned int kmsg;  // if 1, create /dev/kmsg symlink
	char *rcfile;	// Copy of the top level rcfile we read

	// Logfile and logleve can be set in a container config file.
	// Those function as defaults.  The defaults can be overriden
	// by command line.  However we don't want the command line
	// specified values to be saved on c->save_config().  So we
	// store the config file specified values here.
	char *logfile;  // the logfile as specifed in config
	int loglevel;   // loglevel as specifed in config (if any)
	int logfd;

	int inherit_ns_fd[LXC_NS_MAX];

	unsigned int start_auto;
	unsigned int start_delay;
	int start_order;
	struct lxc_list groups;
	int nbd_idx;

	/* unshare the mount namespace in the monitor */
	unsigned int monitor_unshare;

	/* set to true when rootfs has been setup */
	bool rootfs_setup;

	/* list of included files */
	struct lxc_list includes;
	/* config entries which are not "lxc.*" are aliens */
	struct lxc_list aliens;

	/* list of environment variables we'll add to the container when
	 * started */
	struct lxc_list environment;

	/* text representation of the config file */
	char *unexpanded_config;
	size_t unexpanded_len, unexpanded_alloced;

	/* init command */
	char *init_cmd;

	/* if running in a new user namespace, the UID/GID that init and COMMAND
	 * should run under when using lxc-execute */
	uid_t init_uid;
	gid_t init_gid;

	/* indicator if the container will be destroyed on shutdown */
	unsigned int ephemeral;
};

extern int write_id_mapping(enum idtype idtype, pid_t pid, const char *buf,
			    size_t buf_size);

#ifdef HAVE_TLS
extern __thread struct lxc_conf *current_config;
#else
extern struct lxc_conf *current_config;
#endif

int run_lxc_hooks(const char *name, char *hook, struct lxc_conf *conf,
		  const char *lxcpath, char *argv[]);

extern int detect_shared_rootfs(void);

/*
 * Initialize the lxc configuration structure
 */
extern struct lxc_conf *lxc_conf_init(void);
extern void lxc_conf_free(struct lxc_conf *conf);

extern int pin_rootfs(const char *rootfs);
extern int lxc_map_ids(struct lxc_list *idmap, pid_t pid);
extern int lxc_create_tty(const char *name, struct lxc_conf *conf);
extern void lxc_delete_tty(struct lxc_tty_info *tty_info);

extern int lxc_clear_config_network(struct lxc_conf *c);
extern int lxc_clear_nic(struct lxc_conf *c, const char *key);
extern int lxc_clear_config_caps(struct lxc_conf *c);
extern int lxc_clear_config_keepcaps(struct lxc_conf *c);
extern int lxc_clear_cgroups(struct lxc_conf *c, const char *key);
extern int lxc_clear_mount_entries(struct lxc_conf *c);
extern int lxc_clear_automounts(struct lxc_conf *c);
extern int lxc_clear_hooks(struct lxc_conf *c, const char *key);
extern int lxc_clear_idmaps(struct lxc_conf *c);
extern int lxc_clear_groups(struct lxc_conf *c);
extern int lxc_clear_environment(struct lxc_conf *c);
extern int lxc_delete_autodev(struct lxc_handler *handler);
extern void lxc_clear_includes(struct lxc_conf *conf);

extern int do_rootfs_setup(struct lxc_conf *conf, const char *name,
			   const char *lxcpath);

/*
 * Configure the container from inside
 */

struct cgroup_process_info;
extern int lxc_setup(struct lxc_handler *handler);
extern int find_unmapped_nsid(struct lxc_conf *conf, enum idtype idtype);
extern int mapped_hostid(unsigned id, struct lxc_conf *conf, enum idtype idtype);
extern int chown_mapped_root(char *path, struct lxc_conf *conf);
extern int userns_exec_1(struct lxc_conf *conf, int (*fn)(void *), void *data,
			 const char *fn_name);
extern int userns_exec_full(struct lxc_conf *conf, int (*fn)(void *),
			    void *data, const char *fn_name);
extern int parse_mntopts(const char *mntopts, unsigned long *mntflags,
			 char **mntdata);
extern void tmp_proc_unmount(struct lxc_conf *lxc_conf);
void remount_all_slave(void);
extern void suggest_default_idmap(void);
extern FILE *make_anonymous_mount_file(struct lxc_list *mount);
extern struct lxc_list *sort_cgroup_settings(struct lxc_list *cgroup_settings);
extern unsigned long add_required_remount_flags(const char *s, const char *d,
						unsigned long flags);
extern int run_script(const char *name, const char *section, const char *script,
		      ...);
extern int in_caplist(int cap, struct lxc_list *caps);

#endif /* __LXC_CONF_H */
