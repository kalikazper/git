/*
 * GIT - The information manager from hell
 *
 * Copyright (C) Linus Torvalds, 2005
 */
#include "cache.h"
#include "diff.h"

static const char diff_files_usage[] =
"git-diff-files [-q] "
"[<common diff options>] [<path>...]"
COMMON_DIFF_OPTIONS_HELP;

static struct diff_options diff_options;
static int silent = 0;

static void show_unmerge(const char *path)
{
	diff_unmerge(&diff_options, path);
}

static void show_file(int pfx, struct cache_entry *ce)
{
	diff_addremove(&diff_options, pfx, ntohl(ce->ce_mode),
		       ce->sha1, ce->name, NULL);
}

static void show_modified(int oldmode, int mode,
			  const unsigned char *old_sha1, const unsigned char *sha1,
			  char *path)
{
	diff_change(&diff_options, oldmode, mode, old_sha1, sha1, path, NULL);
}

int main(int argc, const char **argv)
{
	static const unsigned char null_sha1[20] = { 0, };
	const char **pathspec;
	const char *prefix = setup_git_directory();
	int entries, i;

	diff_setup(&diff_options);
	while (1 < argc && argv[1][0] == '-') {
		if (!strcmp(argv[1], "-q"))
			silent = 1;
		else if (!strcmp(argv[1], "-r"))
			; /* no-op */
		else if (!strcmp(argv[1], "-s"))
			; /* no-op */
		else {
			int diff_opt_cnt;
			diff_opt_cnt = diff_opt_parse(&diff_options,
						      argv+1, argc-1);
			if (diff_opt_cnt < 0)
				usage(diff_files_usage);
			else if (diff_opt_cnt) {
				argv += diff_opt_cnt;
				argc -= diff_opt_cnt;
				continue;
			}
			else
				usage(diff_files_usage);
		}
		argv++; argc--;
	}

	/* Find the directory, and set up the pathspec */
	pathspec = get_pathspec(prefix, argv + 1);
	entries = read_cache();

	if (diff_setup_done(&diff_options) < 0)
		usage(diff_files_usage);

	/* At this point, if argc == 1, then we are doing everything.
	 * Otherwise argv[1] .. argv[argc-1] have the explicit paths.
	 */
	if (entries < 0) {
		perror("read_cache");
		exit(1);
	}

	for (i = 0; i < entries; i++) {
		struct stat st;
		unsigned int oldmode;
		struct cache_entry *ce = active_cache[i];
		int changed;

		if (!ce_path_match(ce, pathspec))
			continue;

		if (ce_stage(ce)) {
			show_unmerge(ce->name);
			while (i < entries &&
			       !strcmp(ce->name, active_cache[i]->name))
				i++;
			i--; /* compensate for loop control increments */
			continue;
		}

		if (lstat(ce->name, &st) < 0) {
			if (errno != ENOENT && errno != ENOTDIR) {
				perror(ce->name);
				continue;
			}
			if (silent)
				continue;
			show_file('-', ce);
			continue;
		}
		changed = ce_match_stat(ce, &st);
		if (!changed && !diff_options.find_copies_harder)
			continue;
		oldmode = ntohl(ce->ce_mode);
		show_modified(oldmode, DIFF_FILE_CANON_MODE(st.st_mode),
			      ce->sha1, (changed ? null_sha1 : ce->sha1),
			      ce->name);
	}
	diffcore_std(&diff_options);
	diff_flush(&diff_options);
	return 0;
}
