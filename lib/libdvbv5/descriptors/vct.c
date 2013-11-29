/*
 * Copyright (c) 2013 - Mauro Carvalho Chehab <m.chehab@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#include "descriptors/vct.h"
#include "dvb-fe.h"
#include "parse_string.h"

void dvb_table_vct_init(struct dvb_v5_fe_parms *parms, const uint8_t *buf,
			ssize_t buflen, uint8_t *table, ssize_t *table_length)
{
	const uint8_t *p = buf, *endbuf = buf + buflen - 4;
	struct dvb_table_vct *vct = (void *)table;
	struct dvb_table_vct_channel **head = &vct->channel;
	int i, n;
	size_t size = offsetof(struct dvb_table_vct, channel);

	if (p + size > endbuf) {
		dvb_logerr("VCT table was truncated. Need %zu bytes, but has only %zu.",
			   size, buflen);
		return;
	}

	if (*table_length > 0) {
		/* find end of curent list */
		while (*head != NULL)
			head = &(*head)->next;
	} else {
		memcpy(vct, p, size);

		*table_length = sizeof(struct dvb_table_vct);

		vct->channel = NULL;
		vct->descriptor = NULL;
	}
	p += size;

	size = offsetof(struct dvb_table_vct_channel, descriptor);
	for (n = 0; n < vct->num_channels_in_section; n++) {
		struct dvb_table_vct_channel *channel;

		if (p + size > endbuf) {
			dvb_logerr("VCT channel table is missing %d elements",
				   vct->num_channels_in_section - n + 1);
			vct->num_channels_in_section = n;
			break;
		}

		channel = malloc(sizeof(struct dvb_table_vct_channel));

		memcpy(channel, p, size);
		p += size;

		/* Fix endiannes of the copied fields */
		for (i = 0; i < ARRAY_SIZE(channel->__short_name); i++)
			bswap16(channel->__short_name[i]);

		bswap32(channel->carrier_frequency);
		bswap16(channel->channel_tsid);
		bswap16(channel->program_number);
		bswap16(channel->bitfield1);
		bswap16(channel->bitfield2);
		bswap16(channel->source_id);
		bswap16(channel->bitfield3);

		/* Short name is always UTF-16 */
		iconv_to_charset(parms, channel->short_name,
				 sizeof(channel->short_name),
				 (const unsigned char *)channel->__short_name,
				 sizeof(channel->__short_name),
				 "UTF-16",
				 output_charset);

		/* Fill descriptors */

		channel->descriptor = NULL;
		channel->next = NULL;

		*head = channel;
		head = &(*head)->next;

		/* get the descriptors for each program */
		dvb_parse_descriptors(parms, p, channel->descriptors_length,
				      &channel->descriptor);

		p += channel->descriptors_length;
	}

	/* Get extra descriptors */
	size = sizeof(union dvb_table_vct_descriptor_length);
	while (p + size <= endbuf) {
		union dvb_table_vct_descriptor_length *d = (void *)p;
		bswap16(d->descriptor_length);
		p += size;
		dvb_parse_descriptors(parms, p, d->descriptor_length,
				      &vct->descriptor);
	}
	if (endbuf - p)
		dvb_logerr("VCT table has %zu spurious bytes at the end.",
			   endbuf - p);
}

void dvb_table_vct_free(struct dvb_table_vct *vct)
{
	struct dvb_table_vct_channel *channel = vct->channel;
	while(channel) {
		dvb_free_descriptors((struct dvb_desc **) &channel->descriptor);
		struct dvb_table_vct_channel *tmp = channel;
		channel = channel->next;
		free(tmp);
	}
	dvb_free_descriptors((struct dvb_desc **) &vct->descriptor);

	free(vct);
}

void dvb_table_vct_print(struct dvb_v5_fe_parms *parms, struct dvb_table_vct *vct)
{
	if (vct->header.table_id == DVB_TABLE_CVCT)
		dvb_log("CVCT");
	else
		dvb_log("TVCT");

	dvb_table_header_print(parms, &vct->header);

	dvb_log("|- Protocol version %d", vct->ATSC_protocol_version);
	dvb_log("|- #channels        %d", vct->num_channels_in_section);
	dvb_log("|\\  channel_id");
	const struct dvb_table_vct_channel *channel = vct->channel;
	uint16_t channels = 0;
	while(channel) {
		dvb_log("|- Channel                %d.%d: %s",
			channel->major_channel_number,
			channel->minor_channel_number,
			channel->short_name);
		dvb_log("|   modulation mode       %d", channel->modulation_mode);
		dvb_log("|   carrier frequency     %d", channel->carrier_frequency);
		dvb_log("|   TS ID                 %d", channel->channel_tsid);
		dvb_log("|   program number        %d", channel->program_number);

		dvb_log("|   ETM location          %d", channel->ETM_location);
		dvb_log("|   access controlled     %d", channel->access_controlled);
		dvb_log("|   hidden                %d", channel->hidden);

		if (vct->header.table_id == DVB_TABLE_CVCT) {
			dvb_log("|   path select           %d", channel->path_select);
			dvb_log("|   out of band           %d", channel->out_of_band);
		}
		dvb_log("|   hide guide            %d", channel->hide_guide);
		dvb_log("|   service type          %d", channel->service_type);
		dvb_log("|   source id            %d", channel->source_id);

		dvb_print_descriptors(parms, channel->descriptor);
		channel = channel->next;
		channels++;
	}
	dvb_log("|_  %d channels", channels);
}

