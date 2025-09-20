/*
 * This file is part of the OpenTraceCLI project.
Originally derived from opentrace-cli project.
 *
 * Copyright (C) 2024 OpenTraceLab Contributors
Original Copyright (C) 2013 Bert Vermeulen <bert@biot.com>
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

#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include "opentrace-cli.h"

static uint64_t limit_samples = 0;
static uint64_t limit_frames = 0;

#ifdef HAVE_OTD
extern struct srd_session *srd_sess;
#endif

static int set_limit_time(const struct otc_dev_inst *sdi)
{
	GVariant *gvar;
	uint64_t time_msec;
	uint64_t samplerate;
	struct otc_dev_driver *driver;

	driver = otc_dev_inst_driver_get(sdi);

	if (!(time_msec = otc_parse_timestring(opt_time))) {
		g_critical("Invalid time '%s'", opt_time);
		return OTC_ERR;
	}

	if (otc_dev_config_capabilities_list(sdi, NULL, OTC_CONF_LIMIT_MSEC)
			& OTC_CONF_SET) {
		gvar = g_variant_new_uint64(time_msec);
		if (otc_config_set(sdi, NULL, OTC_CONF_LIMIT_MSEC, gvar) != OTC_OK) {
			g_critical("Failed to configure time limit.");
			return OTC_ERR;
		}
	} else if (otc_dev_config_capabilities_list(sdi, NULL, OTC_CONF_SAMPLERATE)
			& (OTC_CONF_GET | OTC_CONF_SET)) {
		/* Convert to samples based on the samplerate. */
		otc_config_get(driver, sdi, NULL, OTC_CONF_SAMPLERATE, &gvar);
		samplerate = g_variant_get_uint64(gvar);
		g_variant_unref(gvar);
		limit_samples = (samplerate) * time_msec / (uint64_t)1000;
		if (limit_samples == 0) {
			g_critical("Not enough time at this samplerate.");
			return OTC_ERR;
		}
		gvar = g_variant_new_uint64(limit_samples);
		if (otc_config_set(sdi, NULL, OTC_CONF_LIMIT_SAMPLES, gvar) != OTC_OK) {
			g_critical("Failed to configure time-based sample limit.");
			return OTC_ERR;
		}
	} else {
		g_critical("This device does not support time limits.");
		return OTC_ERR;
	}

	return OTC_OK;
}

const struct otc_output *setup_output_format(const struct otc_dev_inst *sdi, FILE **outfile)
{
	const struct otc_output_module *omod;
	const struct otc_option **options;
	const struct otc_output *o;
	GHashTable *fmtargs, *fmtopts;
	char *fmtspec;

	if (!opt_output_format) {
		if (opt_output_file) {
			opt_output_format = DEFAULT_OUTPUT_FORMAT_FILE;
		} else {
			opt_output_format = DEFAULT_OUTPUT_FORMAT_NOFILE;
		}
	}

	fmtargs = parse_generic_arg(opt_output_format, TRUE, NULL);
	fmtspec = g_hash_table_lookup(fmtargs, "opentrace_key");
	if (!fmtspec)
		g_critical("Invalid output format.");
	if (!(omod = otc_output_find(fmtspec)))
		g_critical("Unknown output module '%s'.", fmtspec);
	g_hash_table_remove(fmtargs, "opentrace_key");
	if ((options = otc_output_options_get(omod))) {
		fmtopts = generic_arg_to_opt(options, fmtargs);
		(void)warn_unknown_keys(options, fmtargs, NULL);
		otc_output_options_free(options);
	} else {
		fmtopts = NULL;
	}
	o = otc_output_new(omod, fmtopts, sdi, opt_output_file);

	if (opt_output_file) {
		if (!otc_output_test_flag(omod, OTC_OUTPUT_INTERNAL_IO_HANDLING)) {
			*outfile = g_fopen(opt_output_file, "wb");
			if (!*outfile) {
				g_critical("Cannot write to output file '%s'.",
					opt_output_file);
			}
		} else {
			*outfile = NULL;
		}
	} else {
		setup_binary_stdout();
		*outfile = stdout;
	}

	if (fmtopts)
		g_hash_table_destroy(fmtopts);
	g_hash_table_destroy(fmtargs);

	return o;
}

const struct otc_transform *setup_transform_module(const struct otc_dev_inst *sdi)
{
	const struct otc_transform_module *tmod;
	const struct otc_option **options;
	const struct otc_transform *t;
	GHashTable *fmtargs, *fmtopts;
	char *fmtspec;

	fmtargs = parse_generic_arg(opt_transform_module, TRUE, NULL);
	fmtspec = g_hash_table_lookup(fmtargs, "opentrace_key");
	if (!fmtspec)
		g_critical("Invalid transform module.");
	if (!(tmod = otc_transform_find(fmtspec)))
		g_critical("Unknown transform module '%s'.", fmtspec);
	g_hash_table_remove(fmtargs, "opentrace_key");
	if ((options = otc_transform_options_get(tmod))) {
		fmtopts = generic_arg_to_opt(options, fmtargs);
		(void)warn_unknown_keys(options, fmtargs, NULL);
		otc_transform_options_free(options);
	} else {
		fmtopts = NULL;
	}
	t = otc_transform_new(tmod, fmtopts, sdi);
	if (fmtopts)
		g_hash_table_destroy(fmtopts);
	g_hash_table_destroy(fmtargs);

	return t;
}

/* Get the input stream's list of channels and their types, once. */
static void props_get_channels(struct df_arg_desc *args,
	const struct otc_dev_inst *sdi)
{
	struct input_stream_props *props;
	GSList *l;
	const struct otc_channel *ch;

	if (!args)
		return;
	props = &args->props;
	if (props->channels)
		return;

	props->channels = g_slist_copy(otc_dev_inst_channels_get(sdi));
	if (!props->channels)
		return;
	for (l = props->channels; l; l = l->next) {
		ch = l->data;
		if (!ch->enabled)
			continue;
		if (ch->type != OTC_CHANNEL_ANALOG)
			continue;
		props->first_analog_channel = ch;
		break;
	}
}

static gboolean props_chk_1st_channel(struct df_arg_desc *args,
	const struct otc_datafeed_analog *analog)
{
	struct otc_channel *ch;

	if (!args || !analog || !analog->meaning)
		return FALSE;
	ch = g_slist_nth_data(analog->meaning->channels, 0);
	if (!ch)
		return FALSE;
	return ch == args->props.first_analog_channel;
}

static void props_dump_details(struct df_arg_desc *args)
{
	struct input_stream_props *props;
	size_t ch_count;
	GSList *l;
	const struct otc_channel *ch;
	const char *type;

	if (!args)
		return;
	props = &args->props;
	if (props->samplerate)
		printf("Samplerate: %" PRIu64 "\n", props->samplerate);
	if (props->channels) {
		ch_count = g_slist_length(props->channels);
		printf("Channels: %zu\n", ch_count);
		for (l = props->channels; l; l = l->next) {
			ch = l->data;
			if (ch->type == OTC_CHANNEL_ANALOG)
				type = "analog";
			else
				type = "logic";
			printf("- %s: %s\n", ch->name, type);
		}
	}
	if (props->unitsize)
		printf("Logic unitsize: %zu\n", props->unitsize);
	if (props->sample_count_logic)
		printf("Logic sample count: %" PRIu64 "\n", props->sample_count_logic);
	if (props->sample_count_analog)
		printf("Analog sample count: %" PRIu64 "\n", props->sample_count_analog);
	if (props->frame_count)
		printf("Frame count: %" PRIu64 "\n", props->frame_count);
	if (props->triggered)
		printf("Trigger count: %" PRIu64 "\n", props->triggered);
}

static void props_cleanup(struct df_arg_desc *args)
{
	struct input_stream_props *props;

	if (!args)
		return;
	props = &args->props;
	g_slist_free(props->channels);
	props->channels = NULL;
	props->first_analog_channel = NULL;
}

void datafeed_in(const struct otc_dev_inst *sdi,
		const struct otc_datafeed_packet *packet, void *cb_data)
{
	static const struct otc_output *o = NULL;
	static const struct otc_output *oa = NULL;
	static uint64_t rcvd_samples_logic = 0;
	static uint64_t rcvd_samples_analog = 0;
	static uint64_t samplerate = 0;
	static int triggered = 0;
	static FILE *outfile = NULL;

	const struct otc_datafeed_meta *meta;
	const struct otc_datafeed_logic *logic;
	const struct otc_datafeed_analog *analog;
	struct df_arg_desc *df_arg;
	int do_props;
	struct input_stream_props *props;
	struct otc_session *session;
	struct otc_config *src;
	GSList *l;
	GString *out;
	GVariant *gvar;
	uint64_t end_sample;
	uint64_t input_len;
	struct otc_dev_driver *driver;

	/* Avoid warnings when building without decoder support. */
	(void)session;
	(void)input_len;

	driver = otc_dev_inst_driver_get(sdi);

	/* Skip all packets before the first header. */
	if (packet->type != OTC_DF_HEADER && !o)
		return;

	/* Prepare to either process data, or "just" gather properties. */
	df_arg = cb_data;
	session = df_arg->session;
	do_props = df_arg->do_props;
	props = &df_arg->props;

	switch (packet->type) {
	case OTC_DF_HEADER:
		g_debug("cli: Received OTC_DF_HEADER.");
		if (maybe_config_get(driver, sdi, NULL, OTC_CONF_SAMPLERATE,
				&gvar) == OTC_OK) {
			samplerate = g_variant_get_uint64(gvar);
			g_variant_unref(gvar);
		}
		if (do_props) {
			/* Setup variables for maximum code path re-use. */
			o = (void *)-1;
			limit_samples = 0;
			/* Start collecting input stream properties. */
			memset(props, 0, sizeof(*props));
			props->samplerate = samplerate;
			props_get_channels(df_arg, sdi);
			break;
		}
		if (!(o = setup_output_format(sdi, &outfile)))
			g_critical("Failed to initialize output module.");

		/* Set up backup analog output module. */
		if (outfile)
			oa = otc_output_new(otc_output_find("analog"), NULL,
					sdi, NULL);

		rcvd_samples_logic = rcvd_samples_analog = 0;

#ifdef HAVE_OTD
		if (opt_pds) {
			if (samplerate) {
				if (srd_session_metadata_set(srd_sess, OTD_CONF_SAMPLERATE,
						g_variant_new_uint64(samplerate)) != OTD_OK) {
					g_critical("Failed to configure decode session.");
					break;
				}
				pd_samplerate = samplerate;
			}
			if (srd_session_start(srd_sess) != OTD_OK) {
				g_critical("Failed to start decode session.");
				break;
			}
		}
#endif
		break;

	case OTC_DF_META:
		g_debug("cli: Received OTC_DF_META.");
		meta = packet->payload;
		for (l = meta->config; l; l = l->next) {
			src = l->data;
			switch (src->key) {
			case OTC_CONF_SAMPLERATE:
				samplerate = g_variant_get_uint64(src->data);
				g_debug("cli: Got samplerate %"PRIu64" Hz.", samplerate);
				if (do_props) {
					props->samplerate = samplerate;
					break;
				}
#ifdef HAVE_OTD
				if (opt_pds) {
					if (srd_session_metadata_set(srd_sess, OTD_CONF_SAMPLERATE,
							g_variant_new_uint64(samplerate)) != OTD_OK) {
						g_critical("Failed to pass samplerate to decoder.");
					}
					pd_samplerate = samplerate;
				}
#endif
				break;
			case OTC_CONF_SAMPLE_INTERVAL:
				samplerate = g_variant_get_uint64(src->data);
				g_debug("cli: Got sample interval %"PRIu64" ms.", samplerate);
				if (do_props) {
					props->samplerate = samplerate;
					break;
				}
				break;
			default:
				/* Unknown metadata is not an error. */
				break;
			}
		}
		break;

	case OTC_DF_TRIGGER:
		g_debug("cli: Received OTC_DF_TRIGGER.");
		if (do_props) {
			props->triggered++;
			break;
		}
		triggered = 1;
		break;

	case OTC_DF_LOGIC:
		logic = packet->payload;
		g_message("cli: Received OTC_DF_LOGIC (%"PRIu64" bytes, unitsize = %d).",
				logic->length, logic->unitsize);
		if (logic->length == 0)
			break;

		if (do_props) {
			props_get_channels(df_arg, sdi);
			props->unitsize = logic->unitsize;
			props->sample_count_logic += logic->length / logic->unitsize;
			break;
		}

		/* Don't store any samples until triggered. */
		if (opt_wait_trigger && !triggered)
			break;

		if (limit_samples && rcvd_samples_logic >= limit_samples)
			break;

		end_sample = rcvd_samples_logic + logic->length / logic->unitsize;
		/* Cut off last packet according to the sample limit. */
		if (limit_samples && end_sample > limit_samples)
			end_sample = limit_samples;
		input_len = (end_sample - rcvd_samples_logic) * logic->unitsize;

		if (opt_pds) {
#ifdef HAVE_OTD
			if (srd_session_send(srd_sess, rcvd_samples_logic, end_sample,
					logic->data, input_len, logic->unitsize) != OTD_OK)
				otc_session_stop(session);
#endif
		}

		rcvd_samples_logic = end_sample;
		break;

	case OTC_DF_ANALOG:
		analog = packet->payload;
		g_message("cli: Received OTC_DF_ANALOG (%d samples).", analog->num_samples);
		if (analog->num_samples == 0)
			break;

		if (do_props) {
			/* Only count the first analog channel. */
			props_get_channels(df_arg, sdi);
			if (!props_chk_1st_channel(df_arg, analog))
				break;
			props->sample_count_analog += analog->num_samples;
			break;
		}

		if (limit_samples && rcvd_samples_analog >= limit_samples)
			break;

		rcvd_samples_analog += analog->num_samples;
		break;

	case OTC_DF_FRAME_BEGIN:
		g_debug("cli: Received OTC_DF_FRAME_BEGIN.");
		break;

	case OTC_DF_FRAME_END:
		g_debug("cli: Received OTC_DF_FRAME_END.");
		if (do_props) {
			props->frame_count++;
			break;
		}
		break;

	default:
		break;
	}

	if (!do_props && o && !opt_pds) {
		if (otc_output_send(o, packet, &out) == OTC_OK) {
			if (oa && !out) {
				/*
				 * The user didn't specify an output module,
				 * but needs to see this analog data.
				 */
				otc_output_send(oa, packet, &out);
			}
			if (outfile && out && out->len > 0) {
				fwrite(out->str, 1, out->len, outfile);
				fflush(outfile);
			}
			if (out)
				g_string_free(out, TRUE);
		}
	}

	/*
	 * OTC_DF_END needs to be handled after the output module's receive()
	 * is called, so it can properly clean up that module.
	 */
	if (packet->type == OTC_DF_END) {
		g_debug("cli: Received OTC_DF_END.");

#if defined HAVE_OTD_SESSION_SEND_EOF && HAVE_OTD_SESSION_SEND_EOF
		(void)srd_session_send_eof(srd_sess);
#endif

		if (do_props) {
			props_dump_details(df_arg);
			props_cleanup(df_arg);
			o = NULL;
		}

		if (o)
			otc_output_free(o);
		o = NULL;

		if (oa)
			otc_output_free(oa);
		oa = NULL;

		if (outfile && outfile != stdout)
			fclose(outfile);

		if (limit_samples) {
			if (rcvd_samples_logic > 0 && rcvd_samples_logic < limit_samples)
				g_warning("Device only sent %" PRIu64 " samples.",
					   rcvd_samples_logic);
			else if (rcvd_samples_analog > 0 && rcvd_samples_analog < limit_samples)
				g_warning("Device only sent %" PRIu64 " samples.",
					   rcvd_samples_analog);
		}
	}

}

int opt_to_gvar(char *key, char *value, struct otc_config *src)
{
	const struct otc_key_info *srci, *srmqi;
	double tmp_double, dlow, dhigh;
	uint64_t tmp_u64, p, q, low, high, mqflags;
	uint32_t mq;
	GVariant *rational[2], *range[2], *gtup[2];
	GVariantBuilder *vbl;
	gboolean tmp_bool;
	gchar **keyval;
	int ret, i;

	if (!(srci = otc_key_info_name_get(OTC_KEY_CONFIG, key))) {
		g_critical("Unknown device option '%s'.", (char *) key);
		return -1;
	}
	src->key = srci->key;

	if ((!value || strlen(value) == 0) &&
		(srci->datatype != OTC_T_BOOL)) {
		g_critical("Option '%s' needs a value.", (char *)key);
		return -1;
	}

	ret = 0;
	switch (srci->datatype) {
	case OTC_T_UINT64:
		ret = otc_parse_sizestring(value, &tmp_u64);
		if (ret != 0)
			break;
		src->data = g_variant_new_uint64(tmp_u64);
		break;
	case OTC_T_INT32:
		ret = otc_parse_sizestring(value, &tmp_u64);
		if (ret != 0)
			break;
		src->data = g_variant_new_int32(tmp_u64);
		break;
	case OTC_T_STRING:
		src->data = g_variant_new_string(value);
		break;
	case OTC_T_BOOL:
		if (!value)
			tmp_bool = TRUE;
		else
			tmp_bool = otc_parse_boolstring(value);
		src->data = g_variant_new_boolean(tmp_bool);
		break;
	case OTC_T_FLOAT:
		tmp_double = strtof(value, NULL);
		src->data = g_variant_new_double(tmp_double);
		break;
	case OTC_T_RATIONAL_PERIOD:
		if ((ret = otc_parse_period(value, &p, &q)) != OTC_OK)
			break;
		rational[0] = g_variant_new_uint64(p);
		rational[1] = g_variant_new_uint64(q);
		src->data = g_variant_new_tuple(rational, 2);
		break;
	case OTC_T_RATIONAL_VOLT:
		if ((ret = otc_parse_voltage(value, &p, &q)) != OTC_OK)
			break;
		rational[0] = g_variant_new_uint64(p);
		rational[1] = g_variant_new_uint64(q);
		src->data = g_variant_new_tuple(rational, 2);
		break;
	case OTC_T_UINT64_RANGE:
		if (sscanf(value, "%"PRIu64"-%"PRIu64, &low, &high) != 2) {
			ret = -1;
			break;
		} else {
			range[0] = g_variant_new_uint64(low);
			range[1] = g_variant_new_uint64(high);
			src->data = g_variant_new_tuple(range, 2);
		}
		break;
	case OTC_T_DOUBLE_RANGE:
		if (sscanf(value, "%lf-%lf", &dlow, &dhigh) != 2) {
			ret = -1;
			break;
		} else {
			range[0] = g_variant_new_double(dlow);
			range[1] = g_variant_new_double(dhigh);
			src->data = g_variant_new_tuple(range, 2);
		}
		break;
	case OTC_T_KEYVALUE:
		/* Expects the argument to be in the form of key=value. */
		keyval = g_strsplit(value, "=", 2);
		if (!keyval[0] || !keyval[1]) {
			g_strfreev(keyval);
			ret = -1;
			break;
		} else {
			vbl = g_variant_builder_new(G_VARIANT_TYPE_DICTIONARY);
			g_variant_builder_add(vbl, "{ss}",
					      keyval[0], keyval[1]);
			src->data = g_variant_builder_end(vbl);
			g_strfreev(keyval);
		}
		break;
	case OTC_T_MQ:
		/*
		  Argument is MQ id e.g. ("voltage") optionally followed by one
		  or more /<mqflag> e.g. "/ac".
		 */
		keyval = g_strsplit(value, "/", 0);
		if (!keyval[0] || !(srmqi = otc_key_info_name_get(OTC_KEY_MQ, keyval[0]))) {
			g_strfreev(keyval);
			ret = -1;
			break;
		}
		mq = srmqi->key;
		mqflags = 0;
		for (i = 1; keyval[i]; i++) {
			if (!(srmqi = otc_key_info_name_get(OTC_KEY_MQFLAGS, keyval[i]))) {
				ret = -1;
				break;
			}
			mqflags |= srmqi->key;
		}
		g_strfreev(keyval);
		if (ret != -1) {
			gtup[0] = g_variant_new_uint32(mq);
			gtup[1] = g_variant_new_uint64(mqflags);
			src->data = g_variant_new_tuple(gtup, 2);
		}
		break;
	default:
		g_critical("Unknown data type specified for option '%s' "
			   "(driver implementation bug?).", key);
		ret = -1;
	}

	if (ret < 0)
		g_critical("Invalid value: '%s' for option '%s'", value, key);

	return ret;
}

int set_dev_options_array(struct otc_dev_inst *sdi, char **opts)
{
	size_t opt_idx;
	const char *opt_text;
	GHashTable *args;
	int ret;

	for (opt_idx = 0; opts && opts[opt_idx]; opt_idx++) {
		opt_text = opts[opt_idx];
		args = parse_generic_arg(opt_text, FALSE, "channel_group");
		if (!args)
			continue;
		ret = set_dev_options(sdi, args);
		g_hash_table_destroy(args);
		if (ret != OTC_OK)
			return ret;
	}

	return OTC_OK;
}

int set_dev_options(struct otc_dev_inst *sdi, GHashTable *args)
{
	struct otc_config src;
	const char *cg_name;
	struct otc_channel_group *cg;
	GHashTableIter iter;
	gpointer key, value;
	int ret;

	/*
	 * Not finding the 'opentrace_key' key (optional user specified
	 * channel group name) in the current options group's hash table
	 * is perfectly fine. In that case the -g selection is used,
	 * which defaults to "the device" (global parameters).
	 */
	cg_name = g_hash_table_lookup(args, "opentrace_key");
	cg = lookup_channel_group(sdi, cg_name);

	g_hash_table_iter_init(&iter, args);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		if (g_ascii_strcasecmp(key, "opentrace_key") == 0)
			continue;
		if ((ret = opt_to_gvar(key, value, &src)) != 0)
			return ret;
		if ((ret = maybe_config_set(otc_dev_inst_driver_get(sdi), sdi, cg,
				src.key, src.data)) != OTC_OK) {
			g_critical("Failed to set device option '%s': %s.",
				   (char *)key, otc_strerror(ret));
			return ret;
		}
	}

	return OTC_OK;
}

void run_session(void)
{
	struct df_arg_desc df_arg;
	GSList *devices, *real_devices, *sd;
	GVariant *gvar;
	struct otc_session *session;
	struct otc_trigger *trigger;
	struct otc_dev_inst *sdi;
	uint64_t min_samples, max_samples;
	GArray *drv_opts;
	guint i;
	int is_demo_dev;
	struct otc_dev_driver *driver;
	const struct otc_transform *t;
	GMainLoop *main_loop;

	memset(&df_arg, 0, sizeof(df_arg));
	df_arg.do_props = FALSE;

	devices = device_scan();
	if (!devices) {
		g_critical("No devices found.");
		return;
	}

	real_devices = NULL;
	for (sd = devices; sd; sd = sd->next) {
		sdi = sd->data;

		driver = otc_dev_inst_driver_get(sdi);

		if (!(drv_opts = otc_dev_options(driver, NULL, NULL))) {
			g_critical("Failed to query list of driver options.");
			return;
		}

		is_demo_dev = 0;
		for (i = 0; i < drv_opts->len; i++) {
			if (g_array_index(drv_opts, uint32_t, i) == OTC_CONF_DEMO_DEV)
				is_demo_dev = 1;
		}

		g_array_free(drv_opts, TRUE);

		if (!is_demo_dev)
			real_devices = g_slist_append(real_devices, sdi);
	}

	if (g_slist_length(devices) > 1) {
		if (g_slist_length(real_devices) != 1) {
			g_critical("opentrace-cli only supports one device for capturing.");
			return;
		} else {
			/* We only have one non-demo device. */
			g_slist_free(devices);
			devices = real_devices;
			real_devices = NULL;
		}
	}

	/* This is unlikely to happen but it makes static analyzers stop complaining. */
	if (!devices) {
		g_critical("No real devices found.");
		return;
	}

	sdi = devices->data;
	g_slist_free(devices);
	g_slist_free(real_devices);

	otc_session_new(otc_ctx, &session);
	df_arg.session = session;
	otc_session_datafeed_callback_add(session, datafeed_in, &df_arg);
	df_arg.session = NULL;

	if (otc_dev_open(sdi) != OTC_OK) {
		g_critical("Failed to open device.");
		return;
	}

	if (otc_session_dev_add(session, sdi) != OTC_OK) {
		g_critical("Failed to add device to session.");
		otc_session_destroy(session);
		return;
	}

	if (opt_configs) {
		if (set_dev_options_array(sdi, opt_configs) != OTC_OK)
			return;
	}

	if (select_channels(sdi) != OTC_OK) {
		g_critical("Failed to set channels.");
		otc_session_destroy(session);
		return;
	}

	trigger = NULL;
	if (opt_triggers) {
		if (!parse_triggerstring(sdi, opt_triggers, &trigger)) {
			otc_session_destroy(session);
			return;
		}
		if (otc_session_trigger_set(session, trigger) != OTC_OK) {
			otc_session_destroy(session);
			return;
		}
	}

	if (opt_continuous) {
		if (!otc_dev_has_option(sdi, OTC_CONF_CONTINUOUS)) {
			g_critical("This device does not support continuous sampling.");
			otc_session_destroy(session);
			return;
		}
	}

	if (opt_time) {
		if (set_limit_time(sdi) != OTC_OK) {
			otc_session_destroy(session);
			return;
		}
	}

	if (opt_samples) {
		if ((otc_parse_sizestring(opt_samples, &limit_samples) != OTC_OK)) {
			g_critical("Invalid sample limit '%s'.", opt_samples);
			otc_session_destroy(session);
			return;
		}
		if (maybe_config_list(driver, sdi, NULL, OTC_CONF_LIMIT_SAMPLES,
				&gvar) == OTC_OK) {
			/*
			 * The device has no compression, or compression is turned
			 * off, and publishes its sample memory size.
			 */
			g_variant_get(gvar, "(tt)", &min_samples, &max_samples);
			g_variant_unref(gvar);
			if (limit_samples < min_samples) {
				g_critical("The device stores at least %"PRIu64
						" samples with the current settings.", min_samples);
			}
			if (limit_samples > max_samples) {
				g_critical("The device can store only %"PRIu64
						" samples with the current settings.", max_samples);
			}
		}
		gvar = g_variant_new_uint64(limit_samples);
		if (maybe_config_set(otc_dev_inst_driver_get(sdi), sdi, NULL, OTC_CONF_LIMIT_SAMPLES, gvar) != OTC_OK) {
			g_critical("Failed to configure sample limit.");
			otc_session_destroy(session);
			return;
		}
	}

	if (opt_frames) {
		if ((otc_parse_sizestring(opt_frames, &limit_frames) != OTC_OK)) {
			g_critical("Invalid frame limit '%s'.", opt_frames);
			otc_session_destroy(session);
			return;
		}
		gvar = g_variant_new_uint64(limit_frames);
		if (maybe_config_set(otc_dev_inst_driver_get(sdi), sdi, NULL, OTC_CONF_LIMIT_FRAMES, gvar) != OTC_OK) {
			g_critical("Failed to configure frame limit.");
			otc_session_destroy(session);
			return;
		}
	}

	if (opt_transform_module) {
		if (!(t = setup_transform_module(sdi)))
			g_critical("Failed to initialize transform module.");
	}

	main_loop = g_main_loop_new(NULL, FALSE);

	otc_session_stopped_callback_set(session,
		(otc_session_stopped_callback)g_main_loop_quit, main_loop);

	if (otc_session_start(session) != OTC_OK) {
		g_critical("Failed to start session.");
		g_main_loop_unref(main_loop);
		otc_session_destroy(session);
		return;
	}

	if (opt_continuous)
		add_anykey(session);

	g_main_loop_run(main_loop);

	if (opt_continuous)
		clear_anykey();

	if (trigger)
		otc_trigger_free(trigger);

	otc_session_datafeed_callback_remove_all(session);
	g_main_loop_unref(main_loop);
	otc_session_destroy(session);
}
