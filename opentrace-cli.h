/*
 * This file is part of the OpenTraceCLI project.
 * Originally derived from opentrace-cli project.
 *
 * Copyright (C) 2024 OpenTraceLab Contributors
Original Copyright (C) 2024 OpenTraceLab Contributors
 * Original Copyright (C) 2024 OpenTraceLab Contributors
Original Copyright (C) 2011 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENTRACE_CLI_OPENTRACE_CLI_H
#define OPENTRACE_CLI_OPENTRACE_CLI_H

#ifdef HAVE_OTD
/* First, so we avoid a _POSIX_C_SOURCE warning. */
#include <libopentracedecode/libopentracedecode.h>
#endif
#include <libopentrace/libopentrace.h>

#define DEFAULT_OUTPUT_FORMAT_FILE "otczip"
#define DEFAULT_OUTPUT_FORMAT_NOFILE "bits:width=64"

/* main.c */
extern struct otc_context *otc_ctx;
int select_channels(struct otc_dev_inst *sdi);
int maybe_config_get(struct otc_dev_driver *driver,
		const struct otc_dev_inst *sdi, struct otc_channel_group *cg,
		uint32_t key, GVariant **gvar);
int maybe_config_set(struct otc_dev_driver *driver,
		const struct otc_dev_inst *sdi, struct otc_channel_group *cg,
		uint32_t key, GVariant *gvar);
int maybe_config_list(struct otc_dev_driver *driver,
		const struct otc_dev_inst *sdi, struct otc_channel_group *cg,
		uint32_t key, GVariant **gvar);

/* show.c */
void show_version(void);
void show_supported(void);
void show_supported_wiki(void);
void show_dev_list(void);
void show_dev_detail(void);
void show_pd_detail(void);
void show_input(void);
void show_output(void);
void show_transform(void);
void show_serial_ports(void);

/* device.c */
GSList *device_scan(void);
struct otc_channel_group *lookup_channel_group(struct otc_dev_inst *sdi,
	const char *cg_name);

/* session.c */
struct df_arg_desc {
	struct otc_session *session;
	int do_props;
	struct input_stream_props {
		uint64_t samplerate;
		GSList *channels;
		const struct otc_channel *first_analog_channel;
		size_t unitsize;
		uint64_t sample_count_logic;
		uint64_t sample_count_analog;
		uint64_t frame_count;
		uint64_t triggered;
	} props;
};
void datafeed_in(const struct otc_dev_inst *sdi,
		const struct otc_datafeed_packet *packet, void *cb_data);
int opt_to_gvar(char *key, char *value, struct otc_config *src);
int set_dev_options_array(struct otc_dev_inst *sdi, char **opts);
int set_dev_options(struct otc_dev_inst *sdi, GHashTable *args);
void run_session(void);

/* input.c */
void load_input_file(gboolean do_props);

/* output.c */
int setup_binary_stdout(void);

/* decode.c */
#ifdef HAVE_OTD
extern uint64_t pd_samplerate;
int register_pds(gchar **all_pds, char *opt_pd_annotations);
int setup_pd_annotations(char *opt_pd_annotations);
int setup_pd_meta(char *opt_pd_meta);
int setup_pd_binary(char *opt_pd_binary);
void show_pd_annotations(struct srd_proto_data *pdata, void *cb_data);
void show_pd_meta(struct srd_proto_data *pdata, void *cb_data);
void show_pd_binary(struct srd_proto_data *pdata, void *cb_data);
void show_pd_prepare(void);
void show_pd_close(void);
void map_pd_channels(struct otc_dev_inst *sdi);
#endif

/* parsers.c */
struct otc_channel *find_channel(GSList *channellist, const char *channelname,
	gboolean exact_case);
GSList *parse_channelstring(struct otc_dev_inst *sdi, const char *channelstring);
int parse_triggerstring(const struct otc_dev_inst *sdi, const char *s,
		struct otc_trigger **trigger);
GHashTable *parse_generic_arg(const char *arg,
		gboolean sep_first, const char *key_first);
GHashTable *generic_arg_to_opt(const struct otc_option **opts, GHashTable *genargs);
GSList *check_unknown_keys(const struct otc_option **avail, GHashTable *used);
gboolean warn_unknown_keys(const struct otc_option **avail, GHashTable *used,
		const char *caption);
int canon_cmp(const char *str1, const char *str2);
int parse_driver(char *arg, struct otc_dev_driver **driver, GSList **drvopts);

/* anykey.c */
void add_anykey(struct otc_session *session);
void clear_anykey(void);

/* options.c */
extern gboolean opt_version;
extern gboolean opt_list_supported;
extern gboolean opt_list_supported_wiki;
extern gint opt_loglevel;
extern gboolean opt_scan_devs;
extern gboolean opt_dont_scan;
extern gboolean opt_wait_trigger;
extern gchar *opt_input_file;
extern gchar *opt_output_file;
extern gchar *opt_drv;
extern gchar **opt_configs;
extern gchar *opt_channels;
extern gchar *opt_channel_group;
extern gchar *opt_triggers;
extern gchar **opt_pds;
#ifdef HAVE_OTD
extern gchar *opt_pd_annotations;
extern gchar *opt_pd_meta;
extern gchar *opt_pd_binary;
extern gboolean opt_pd_ann_class;
extern gboolean opt_pd_samplenum;
extern gboolean opt_pd_jsontrace;
#endif
extern gchar *opt_input_format;
extern gchar *opt_output_format;
extern gchar *opt_transform_module;
extern gboolean opt_show;
extern gchar *opt_time;
extern gchar *opt_samples;
extern gchar *opt_frames;
extern gboolean opt_continuous;
extern gchar **opt_gets;
extern gboolean opt_set;
extern gboolean opt_list_serial;
int parse_options(int argc, char **argv);
void show_help(void);

#endif
