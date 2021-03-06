/* test_libFLAC - Unit tester for libFLAC
 * Copyright (C) 2002  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "encoders.h"
#include "file_utils.h"
#include "FLAC/assert.h"
#include "FLAC/file_encoder.h"
#include "FLAC/seekable_stream_encoder.h"
#include "FLAC/stream_encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_;
static FLAC__StreamMetadata *metadata_sequence_[] = { &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_ };
static const unsigned num_metadata_ = 5;
static const char *flacfilename_ = "metadata.flac";

static FLAC__bool die_s_(const char *msg, const FLAC__StreamEncoder *encoder)
{
	FLAC__StreamEncoderState state = FLAC__stream_encoder_get_state(encoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, FLAC__StreamEncoderStateString[state]);
	if(state == FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
		FLAC__StreamDecoderState dstate = FLAC__stream_encoder_get_verify_decoder_state(encoder);
		printf("      verify decoder state = %u (%s)\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);
	}

	return false;
}

static FLAC__bool die_ss_(const char *msg, const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__SeekableStreamEncoderState state = FLAC__seekable_stream_encoder_get_state(encoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, FLAC__SeekableStreamEncoderStateString[state]);
	if(state == FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR) {
		FLAC__StreamEncoderState state_ = FLAC__seekable_stream_encoder_get_stream_encoder_state(encoder);
		printf("      stream encoder state = %u (%s)\n", (unsigned)state_, FLAC__StreamEncoderStateString[state_]);
		if(state_ == FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
			FLAC__StreamDecoderState dstate = FLAC__seekable_stream_encoder_get_verify_decoder_state(encoder);
			printf("      verify decoder state = %u (%s)\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);
		}
	}

	return false;
}

static FLAC__bool die_f_(const char *msg, const FLAC__FileEncoder *encoder)
{
	FLAC__FileEncoderState state = FLAC__file_encoder_get_state(encoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, FLAC__FileEncoderStateString[state]);
	if(state == FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR) {
		FLAC__SeekableStreamEncoderState state_ = FLAC__file_encoder_get_seekable_stream_encoder_state(encoder);
		printf("      seekable stream encoder state = %u (%s)\n", (unsigned)state_, FLAC__SeekableStreamEncoderStateString[state_]);
		if(state_ == FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR) {
			FLAC__StreamEncoderState state__ = FLAC__file_encoder_get_stream_encoder_state(encoder);
			printf("      stream encoder state = %u (%s)\n", (unsigned)state__, FLAC__StreamEncoderStateString[state__]);
			if(state__ == FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
				FLAC__StreamDecoderState dstate = FLAC__file_encoder_get_verify_decoder_state(encoder);
				printf("      verify decoder state = %u (%s)\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);
			}
		}
	}

	return false;
}

static void *malloc_or_die_(size_t size)
{
	void *x = malloc(size);
	if(0 == x) {
		fprintf(stderr, "ERROR: out of memory allocating %u bytes\n", (unsigned)size);
		exit(1);
	}
	return x;
}

static void init_metadata_blocks_()
{
	/*
		most of the actual numbers and data in the blocks don't matter,
		we just want to make sure the encoder encodes them correctly

		remember, the metadata interface gets tested after the encoders,
		so we do all the metadata manipulation here without it.
	*/

	/* min/max_framesize and md5sum don't get written at first, so we have to leave them 0 */
	streaminfo_.is_last = false;
	streaminfo_.type = FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo_.length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo_.data.stream_info.min_blocksize = 576;
	streaminfo_.data.stream_info.max_blocksize = 576;
	streaminfo_.data.stream_info.min_framesize = 0;
	streaminfo_.data.stream_info.max_framesize = 0;
	streaminfo_.data.stream_info.sample_rate = 44100;
	streaminfo_.data.stream_info.channels = 1;
	streaminfo_.data.stream_info.bits_per_sample = 8;
	streaminfo_.data.stream_info.total_samples = 0;
	memset(streaminfo_.data.stream_info.md5sum, 0, 16);

	padding_.is_last = false;
	padding_.type = FLAC__METADATA_TYPE_PADDING;
	padding_.length = 1234;

	seektable_.is_last = false;
	seektable_.type = FLAC__METADATA_TYPE_SEEKTABLE;
	seektable_.data.seek_table.num_points = 2;
	seektable_.length = seektable_.data.seek_table.num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	seektable_.data.seek_table.points = malloc_or_die_(seektable_.data.seek_table.num_points * sizeof(FLAC__StreamMetadata_SeekPoint));
	seektable_.data.seek_table.points[0].sample_number = 0;
	seektable_.data.seek_table.points[0].stream_offset = 0;
	seektable_.data.seek_table.points[0].frame_samples = streaminfo_.data.stream_info.min_blocksize;
	seektable_.data.seek_table.points[1].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seektable_.data.seek_table.points[1].stream_offset = 1000;
	seektable_.data.seek_table.points[1].frame_samples = streaminfo_.data.stream_info.min_blocksize;

	application1_.is_last = false;
	application1_.type = FLAC__METADATA_TYPE_APPLICATION;
	application1_.length = 8;
	memcpy(application1_.data.application.id, "\xfe\xdc\xba\x98", 4);
	application1_.data.application.data = malloc_or_die_(4);
	memcpy(application1_.data.application.data, "\xf0\xe1\xd2\xc3", 4);

	application2_.is_last = false;
	application2_.type = FLAC__METADATA_TYPE_APPLICATION;
	application2_.length = 4;
	memcpy(application2_.data.application.id, "\x76\x54\x32\x10", 4);
	application2_.data.application.data = 0;

	vorbiscomment_.is_last = true;
	vorbiscomment_.type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
	vorbiscomment_.length = (4 + 8) + 4 + (4 + 5) + (4 + 0);
	vorbiscomment_.data.vorbis_comment.vendor_string.length = 8;
	vorbiscomment_.data.vorbis_comment.vendor_string.entry = malloc_or_die_(8);
	memcpy(vorbiscomment_.data.vorbis_comment.vendor_string.entry, "flac 1.x", 8);
	vorbiscomment_.data.vorbis_comment.num_comments = 2;
	vorbiscomment_.data.vorbis_comment.comments = malloc_or_die_(vorbiscomment_.data.vorbis_comment.num_comments * sizeof(FLAC__StreamMetadata_VorbisComment_Entry));
	vorbiscomment_.data.vorbis_comment.comments[0].length = 5;
	vorbiscomment_.data.vorbis_comment.comments[0].entry = malloc_or_die_(5);
	memcpy(vorbiscomment_.data.vorbis_comment.comments[0].entry, "ab=cd", 5);
	vorbiscomment_.data.vorbis_comment.comments[1].length = 0;
	vorbiscomment_.data.vorbis_comment.comments[1].entry = 0;
}

static void free_metadata_blocks_()
{
	free(seektable_.data.seek_table.points);
	free(application1_.data.application.data);
	free(vorbiscomment_.data.vorbis_comment.vendor_string.entry);
	free(vorbiscomment_.data.vorbis_comment.comments[0].entry);
	free(vorbiscomment_.data.vorbis_comment.comments);
}

static FLAC__StreamEncoderWriteStatus stream_encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	(void)encoder, (void)buffer, (void)bytes, (void)samples, (void)current_frame, (void)client_data;
	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static void stream_encoder_metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)encoder, (void)metadata, (void)client_data;
}

static FLAC__bool test_stream_encoder()
{
	FLAC__StreamEncoder *encoder;
	FLAC__StreamEncoderState state;
	FLAC__StreamDecoderState dstate;
	FLAC__int32 samples[1024];
	FLAC__int32 *samples_array[1] = { samples };
	unsigned i;

	printf("\n+++ libFLAC unit test: FLAC__StreamEncoder\n\n");

	printf("testing FLAC__stream_encoder_new()... ");
	encoder = FLAC__stream_encoder_new();
	if(0 == encoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_verify()... ");
	if(!FLAC__stream_encoder_set_verify(encoder, true))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_streamable_subset()... ");
	if(!FLAC__stream_encoder_set_streamable_subset(encoder, true))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_mid_side_stereo()... ");
	if(!FLAC__stream_encoder_set_do_mid_side_stereo(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_loose_mid_side_stereo()... ");
	if(!FLAC__stream_encoder_set_loose_mid_side_stereo(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_channels()... ");
	if(!FLAC__stream_encoder_set_channels(encoder, streaminfo_.data.stream_info.channels))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_bits_per_sample()... ");
	if(!FLAC__stream_encoder_set_bits_per_sample(encoder, streaminfo_.data.stream_info.bits_per_sample))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_sample_rate()... ");
	if(!FLAC__stream_encoder_set_sample_rate(encoder, streaminfo_.data.stream_info.sample_rate))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_blocksize()... ");
	if(!FLAC__stream_encoder_set_blocksize(encoder, streaminfo_.data.stream_info.min_blocksize))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_max_lpc_order()... ");
	if(!FLAC__stream_encoder_set_max_lpc_order(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_qlp_coeff_precision()... ");
	if(!FLAC__stream_encoder_set_qlp_coeff_precision(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_qlp_coeff_prec_search()... ");
	if(!FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_escape_coding()... ");
	if(!FLAC__stream_encoder_set_do_escape_coding(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_exhaustive_model_search()... ");
	if(!FLAC__stream_encoder_set_do_exhaustive_model_search(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_min_residual_partition_order()... ");
	if(!FLAC__stream_encoder_set_min_residual_partition_order(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_max_residual_partition_order()... ");
	if(!FLAC__stream_encoder_set_max_residual_partition_order(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_rice_parameter_search_dist()... ");
	if(!FLAC__stream_encoder_set_rice_parameter_search_dist(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_total_samples_estimate()... ");
	if(!FLAC__stream_encoder_set_total_samples_estimate(encoder, streaminfo_.data.stream_info.total_samples))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_metadata()... ");
	if(!FLAC__stream_encoder_set_metadata(encoder, metadata_sequence_, num_metadata_))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_write_callback()... ");
	if(!FLAC__stream_encoder_set_write_callback(encoder, stream_encoder_write_callback_))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_metadata_callback()... ");
	if(!FLAC__stream_encoder_set_metadata_callback(encoder, stream_encoder_metadata_callback_))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_client_data()... ");
	if(!FLAC__stream_encoder_set_client_data(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_init()... ");
	if(FLAC__stream_encoder_init(encoder) != FLAC__STREAM_ENCODER_OK)
		return die_s_(0, encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_state()... ");
	state = FLAC__stream_encoder_get_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)state, FLAC__StreamEncoderStateString[state]);

	printf("testing FLAC__stream_encoder_get_verify_decoder_state()... ");
	dstate = FLAC__stream_encoder_get_verify_decoder_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);

	{
		FLAC__uint64 absolute_sample;
		unsigned frame_number;
		unsigned channel;
		unsigned sample;
		FLAC__int32 expected;
		FLAC__int32 got;

		printf("testing FLAC__stream_encoder_get_verify_decoder_error_stats()... ");
		FLAC__stream_encoder_get_verify_decoder_error_stats(encoder, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
		printf("OK\n");
	}

	printf("testing FLAC__stream_encoder_get_verify()... ");
	if(FLAC__stream_encoder_get_verify(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_streamable_subset()... ");
	if(FLAC__stream_encoder_get_streamable_subset(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_mid_side_stereo()... ");
	if(FLAC__stream_encoder_get_do_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_loose_mid_side_stereo()... ");
	if(FLAC__stream_encoder_get_loose_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_channels()... ");
	if(FLAC__stream_encoder_get_channels(encoder) != streaminfo_.data.stream_info.channels) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.channels, FLAC__stream_encoder_get_channels(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_bits_per_sample()... ");
	if(FLAC__stream_encoder_get_bits_per_sample(encoder) != streaminfo_.data.stream_info.bits_per_sample) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.bits_per_sample, FLAC__stream_encoder_get_bits_per_sample(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_sample_rate()... ");
	if(FLAC__stream_encoder_get_sample_rate(encoder) != streaminfo_.data.stream_info.sample_rate) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.sample_rate, FLAC__stream_encoder_get_sample_rate(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_blocksize()... ");
	if(FLAC__stream_encoder_get_blocksize(encoder) != streaminfo_.data.stream_info.min_blocksize) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.min_blocksize, FLAC__stream_encoder_get_blocksize(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_max_lpc_order()... ");
	if(FLAC__stream_encoder_get_max_lpc_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_max_lpc_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_qlp_coeff_precision()... ");
	(void)FLAC__stream_encoder_get_qlp_coeff_precision(encoder);
	/* we asked the encoder to auto select this so we accept anything */
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_qlp_coeff_prec_search()... ");
	if(FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_escape_coding()... ");
	if(FLAC__stream_encoder_get_do_escape_coding(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_exhaustive_model_search()... ");
	if(FLAC__stream_encoder_get_do_exhaustive_model_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_min_residual_partition_order()... ");
	if(FLAC__stream_encoder_get_min_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_min_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_max_residual_partition_order()... ");
	if(FLAC__stream_encoder_get_max_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_max_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_rice_parameter_search_dist()... ");
	if(FLAC__stream_encoder_get_rice_parameter_search_dist(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_rice_parameter_search_dist(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_total_samples_estimate()... ");
	if(FLAC__stream_encoder_get_total_samples_estimate(encoder) != streaminfo_.data.stream_info.total_samples) {
		printf("FAILED, expected %llu, got %llu\n", streaminfo_.data.stream_info.total_samples, FLAC__stream_encoder_get_total_samples_estimate(encoder));
		return false;
	}
	printf("OK\n");

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	printf("testing FLAC__stream_encoder_process()... ");
	if(!FLAC__stream_encoder_process(encoder, (const FLAC__int32 * const *)samples_array, sizeof(samples) / sizeof(FLAC__int32)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_process_interleaved()... ");
	if(!FLAC__stream_encoder_process_interleaved(encoder, samples, sizeof(samples) / sizeof(FLAC__int32)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_finish()... ");
	FLAC__stream_encoder_finish(encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_delete()... ");
	FLAC__stream_encoder_delete(encoder);
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

FLAC__SeekableStreamEncoderSeekStatus seekable_stream_encoder_seek_callback_(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	(void)encoder, (void)absolute_byte_offset, (void)client_data;
	return FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK;
}

FLAC__StreamEncoderWriteStatus seekable_stream_encoder_write_callback_(const FLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	(void)encoder, (void)buffer, (void)bytes, (void)samples, (void)current_frame, (void)client_data;
	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static FLAC__bool test_seekable_stream_encoder()
{
	FLAC__SeekableStreamEncoder *encoder;
	FLAC__SeekableStreamEncoderState state;
	FLAC__StreamEncoderState state_;
	FLAC__StreamDecoderState dstate;
	FLAC__int32 samples[1024];
	FLAC__int32 *samples_array[1] = { samples };
	unsigned i;

	printf("\n+++ libFLAC unit test: FLAC__SeekableStreamEncoder\n\n");

	printf("testing FLAC__seekable_stream_encoder_new()... ");
	encoder = FLAC__seekable_stream_encoder_new();
	if(0 == encoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_verify()... ");
	if(!FLAC__seekable_stream_encoder_set_verify(encoder, true))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_streamable_subset()... ");
	if(!FLAC__seekable_stream_encoder_set_streamable_subset(encoder, true))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_do_mid_side_stereo()... ");
	if(!FLAC__seekable_stream_encoder_set_do_mid_side_stereo(encoder, false))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_loose_mid_side_stereo()... ");
	if(!FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(encoder, false))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_channels()... ");
	if(!FLAC__seekable_stream_encoder_set_channels(encoder, streaminfo_.data.stream_info.channels))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_bits_per_sample()... ");
	if(!FLAC__seekable_stream_encoder_set_bits_per_sample(encoder, streaminfo_.data.stream_info.bits_per_sample))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_sample_rate()... ");
	if(!FLAC__seekable_stream_encoder_set_sample_rate(encoder, streaminfo_.data.stream_info.sample_rate))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_blocksize()... ");
	if(!FLAC__seekable_stream_encoder_set_blocksize(encoder, streaminfo_.data.stream_info.min_blocksize))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_max_lpc_order()... ");
	if(!FLAC__seekable_stream_encoder_set_max_lpc_order(encoder, 0))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_qlp_coeff_precision()... ");
	if(!FLAC__seekable_stream_encoder_set_qlp_coeff_precision(encoder, 0))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search()... ");
	if(!FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(encoder, false))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_do_escape_coding()... ");
	if(!FLAC__seekable_stream_encoder_set_do_escape_coding(encoder, false))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_do_exhaustive_model_search()... ");
	if(!FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(encoder, false))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_min_residual_partition_order()... ");
	if(!FLAC__seekable_stream_encoder_set_min_residual_partition_order(encoder, 0))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_max_residual_partition_order()... ");
	if(!FLAC__seekable_stream_encoder_set_max_residual_partition_order(encoder, 0))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_rice_parameter_search_dist()... ");
	if(!FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(encoder, 0))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_total_samples_estimate()... ");
	if(!FLAC__seekable_stream_encoder_set_total_samples_estimate(encoder, streaminfo_.data.stream_info.total_samples))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_metadata()... ");
	if(!FLAC__seekable_stream_encoder_set_metadata(encoder, metadata_sequence_, num_metadata_))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_seek_callback()... ");
	if(!FLAC__seekable_stream_encoder_set_seek_callback(encoder, seekable_stream_encoder_seek_callback_))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_write_callback()... ");
	if(!FLAC__seekable_stream_encoder_set_write_callback(encoder, seekable_stream_encoder_write_callback_))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_set_client_data()... ");
	if(!FLAC__seekable_stream_encoder_set_client_data(encoder, 0))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_init()... ");
	if(FLAC__seekable_stream_encoder_init(encoder) != FLAC__SEEKABLE_STREAM_ENCODER_OK)
		return die_ss_(0, encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_state()... ");
	state = FLAC__seekable_stream_encoder_get_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)state, FLAC__SeekableStreamEncoderStateString[state]);

	printf("testing FLAC__seekable_stream_encoder_get_stream_encoder_state()... ");
	state_ = FLAC__seekable_stream_encoder_get_stream_encoder_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)state_, FLAC__StreamEncoderStateString[state_]);

	printf("testing FLAC__seekable_stream_encoder_get_verify_decoder_state()... ");
	dstate = FLAC__seekable_stream_encoder_get_verify_decoder_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);

	{
		FLAC__uint64 absolute_sample;
		unsigned frame_number;
		unsigned channel;
		unsigned sample;
		FLAC__int32 expected;
		FLAC__int32 got;

		printf("testing FLAC__seekable_stream_encoder_get_verify_decoder_error_stats()... ");
		FLAC__seekable_stream_encoder_get_verify_decoder_error_stats(encoder, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
		printf("OK\n");
	}

	printf("testing FLAC__seekable_stream_encoder_get_verify()... ");
	if(FLAC__seekable_stream_encoder_get_verify(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_streamable_subset()... ");
	if(FLAC__seekable_stream_encoder_get_streamable_subset(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_do_mid_side_stereo()... ");
	if(FLAC__seekable_stream_encoder_get_do_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_loose_mid_side_stereo()... ");
	if(FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_channels()... ");
	if(FLAC__seekable_stream_encoder_get_channels(encoder) != streaminfo_.data.stream_info.channels) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.channels, FLAC__seekable_stream_encoder_get_channels(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_bits_per_sample()... ");
	if(FLAC__seekable_stream_encoder_get_bits_per_sample(encoder) != streaminfo_.data.stream_info.bits_per_sample) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.bits_per_sample, FLAC__seekable_stream_encoder_get_bits_per_sample(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_sample_rate()... ");
	if(FLAC__seekable_stream_encoder_get_sample_rate(encoder) != streaminfo_.data.stream_info.sample_rate) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.sample_rate, FLAC__seekable_stream_encoder_get_sample_rate(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_blocksize()... ");
	if(FLAC__seekable_stream_encoder_get_blocksize(encoder) != streaminfo_.data.stream_info.min_blocksize) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.min_blocksize, FLAC__seekable_stream_encoder_get_blocksize(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_max_lpc_order()... ");
	if(FLAC__seekable_stream_encoder_get_max_lpc_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__seekable_stream_encoder_get_max_lpc_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_qlp_coeff_precision()... ");
	(void)FLAC__seekable_stream_encoder_get_qlp_coeff_precision(encoder);
	/* we asked the encoder to auto select this so we accept anything */
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search()... ");
	if(FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_do_escape_coding()... ");
	if(FLAC__seekable_stream_encoder_get_do_escape_coding(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_do_exhaustive_model_search()... ");
	if(FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_min_residual_partition_order()... ");
	if(FLAC__seekable_stream_encoder_get_min_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__seekable_stream_encoder_get_min_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_max_residual_partition_order()... ");
	if(FLAC__seekable_stream_encoder_get_max_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__seekable_stream_encoder_get_max_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_rice_parameter_search_dist()... ");
	if(FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_get_total_samples_estimate()... ");
	if(FLAC__seekable_stream_encoder_get_total_samples_estimate(encoder) != streaminfo_.data.stream_info.total_samples) {
		printf("FAILED, expected %llu, got %llu\n", streaminfo_.data.stream_info.total_samples, FLAC__seekable_stream_encoder_get_total_samples_estimate(encoder));
		return false;
	}
	printf("OK\n");

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	printf("testing FLAC__seekable_stream_encoder_process()... ");
	if(!FLAC__seekable_stream_encoder_process(encoder, (const FLAC__int32 * const *)samples_array, sizeof(samples) / sizeof(FLAC__int32)))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_process_interleaved()... ");
	if(!FLAC__seekable_stream_encoder_process_interleaved(encoder, samples, sizeof(samples) / sizeof(FLAC__int32)))
		return die_ss_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_finish()... ");
	FLAC__seekable_stream_encoder_finish(encoder);
	printf("OK\n");

	printf("testing FLAC__seekable_stream_encoder_delete()... ");
	FLAC__seekable_stream_encoder_delete(encoder);
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

static void file_encoder_progress_callback_(const FLAC__FileEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
{
	(void)encoder, (void)bytes_written, (void)samples_written, (void)frames_written, (void)total_frames_estimate, (void)client_data;
}

static FLAC__bool test_file_encoder()
{
	FLAC__FileEncoder *encoder;
	FLAC__FileEncoderState state;
	FLAC__SeekableStreamEncoderState state_;
	FLAC__StreamEncoderState state__;
	FLAC__StreamDecoderState dstate;
	FLAC__int32 samples[1024];
	FLAC__int32 *samples_array[1] = { samples };
	unsigned i;

	printf("\n+++ libFLAC unit test: FLAC__FileEncoder\n\n");

	printf("testing FLAC__file_encoder_new()... ");
	encoder = FLAC__file_encoder_new();
	if(0 == encoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_verify()... ");
	if(!FLAC__file_encoder_set_verify(encoder, true))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_streamable_subset()... ");
	if(!FLAC__file_encoder_set_streamable_subset(encoder, true))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_do_mid_side_stereo()... ");
	if(!FLAC__file_encoder_set_do_mid_side_stereo(encoder, false))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_loose_mid_side_stereo()... ");
	if(!FLAC__file_encoder_set_loose_mid_side_stereo(encoder, false))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_channels()... ");
	if(!FLAC__file_encoder_set_channels(encoder, streaminfo_.data.stream_info.channels))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_bits_per_sample()... ");
	if(!FLAC__file_encoder_set_bits_per_sample(encoder, streaminfo_.data.stream_info.bits_per_sample))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_sample_rate()... ");
	if(!FLAC__file_encoder_set_sample_rate(encoder, streaminfo_.data.stream_info.sample_rate))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_blocksize()... ");
	if(!FLAC__file_encoder_set_blocksize(encoder, streaminfo_.data.stream_info.min_blocksize))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_max_lpc_order()... ");
	if(!FLAC__file_encoder_set_max_lpc_order(encoder, 0))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_qlp_coeff_precision()... ");
	if(!FLAC__file_encoder_set_qlp_coeff_precision(encoder, 0))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_do_qlp_coeff_prec_search()... ");
	if(!FLAC__file_encoder_set_do_qlp_coeff_prec_search(encoder, false))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_do_escape_coding()... ");
	if(!FLAC__file_encoder_set_do_escape_coding(encoder, false))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_do_exhaustive_model_search()... ");
	if(!FLAC__file_encoder_set_do_exhaustive_model_search(encoder, false))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_min_residual_partition_order()... ");
	if(!FLAC__file_encoder_set_min_residual_partition_order(encoder, 0))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_max_residual_partition_order()... ");
	if(!FLAC__file_encoder_set_max_residual_partition_order(encoder, 0))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_rice_parameter_search_dist()... ");
	if(!FLAC__file_encoder_set_rice_parameter_search_dist(encoder, 0))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_total_samples_estimate()... ");
	if(!FLAC__file_encoder_set_total_samples_estimate(encoder, streaminfo_.data.stream_info.total_samples))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_metadata()... ");
	if(!FLAC__file_encoder_set_metadata(encoder, metadata_sequence_, num_metadata_))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_filename()... ");
	if(!FLAC__file_encoder_set_filename(encoder, flacfilename_))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_progress_callback()... ");
	if(!FLAC__file_encoder_set_progress_callback(encoder, file_encoder_progress_callback_))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_set_client_data()... ");
	if(!FLAC__file_encoder_set_client_data(encoder, 0))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_init()... ");
	if(FLAC__file_encoder_init(encoder) != FLAC__FILE_ENCODER_OK)
		return die_f_(0, encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_state()... ");
	state = FLAC__file_encoder_get_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)state, FLAC__FileEncoderStateString[state]);

	printf("testing FLAC__file_encoder_get_seekable_stream_encoder_state()... ");
	state_ = FLAC__file_encoder_get_seekable_stream_encoder_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)state_, FLAC__SeekableStreamEncoderStateString[state_]);

	printf("testing FLAC__file_encoder_get_stream_encoder_state()... ");
	state__ = FLAC__file_encoder_get_stream_encoder_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)state__, FLAC__StreamEncoderStateString[state__]);

	printf("testing FLAC__file_encoder_get_verify_decoder_state()... ");
	dstate = FLAC__file_encoder_get_verify_decoder_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);

	{
		FLAC__uint64 absolute_sample;
		unsigned frame_number;
		unsigned channel;
		unsigned sample;
		FLAC__int32 expected;
		FLAC__int32 got;

		printf("testing FLAC__file_encoder_get_verify_decoder_error_stats()... ");
		FLAC__file_encoder_get_verify_decoder_error_stats(encoder, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
		printf("OK\n");
	}

	printf("testing FLAC__file_encoder_get_verify()... ");
	if(FLAC__file_encoder_get_verify(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_streamable_subset()... ");
	if(FLAC__file_encoder_get_streamable_subset(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_do_mid_side_stereo()... ");
	if(FLAC__file_encoder_get_do_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_loose_mid_side_stereo()... ");
	if(FLAC__file_encoder_get_loose_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_channels()... ");
	if(FLAC__file_encoder_get_channels(encoder) != streaminfo_.data.stream_info.channels) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.channels, FLAC__file_encoder_get_channels(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_bits_per_sample()... ");
	if(FLAC__file_encoder_get_bits_per_sample(encoder) != streaminfo_.data.stream_info.bits_per_sample) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.bits_per_sample, FLAC__file_encoder_get_bits_per_sample(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_sample_rate()... ");
	if(FLAC__file_encoder_get_sample_rate(encoder) != streaminfo_.data.stream_info.sample_rate) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.sample_rate, FLAC__file_encoder_get_sample_rate(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_blocksize()... ");
	if(FLAC__file_encoder_get_blocksize(encoder) != streaminfo_.data.stream_info.min_blocksize) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.min_blocksize, FLAC__file_encoder_get_blocksize(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_max_lpc_order()... ");
	if(FLAC__file_encoder_get_max_lpc_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__file_encoder_get_max_lpc_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_qlp_coeff_precision()... ");
	(void)FLAC__file_encoder_get_qlp_coeff_precision(encoder);
	/* we asked the encoder to auto select this so we accept anything */
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_do_qlp_coeff_prec_search()... ");
	if(FLAC__file_encoder_get_do_qlp_coeff_prec_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_do_escape_coding()... ");
	if(FLAC__file_encoder_get_do_escape_coding(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_do_exhaustive_model_search()... ");
	if(FLAC__file_encoder_get_do_exhaustive_model_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_min_residual_partition_order()... ");
	if(FLAC__file_encoder_get_min_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__file_encoder_get_min_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_max_residual_partition_order()... ");
	if(FLAC__file_encoder_get_max_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__file_encoder_get_max_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_rice_parameter_search_dist()... ");
	if(FLAC__file_encoder_get_rice_parameter_search_dist(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__file_encoder_get_rice_parameter_search_dist(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_encoder_get_total_samples_estimate()... ");
	if(FLAC__file_encoder_get_total_samples_estimate(encoder) != streaminfo_.data.stream_info.total_samples) {
		printf("FAILED, expected %llu, got %llu\n", streaminfo_.data.stream_info.total_samples, FLAC__file_encoder_get_total_samples_estimate(encoder));
		return false;
	}
	printf("OK\n");

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	printf("testing FLAC__file_encoder_process()... ");
	if(!FLAC__file_encoder_process(encoder, (const FLAC__int32 * const *)samples_array, sizeof(samples) / sizeof(FLAC__int32)))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_process_interleaved()... ");
	if(!FLAC__file_encoder_process_interleaved(encoder, samples, sizeof(samples) / sizeof(FLAC__int32)))
		return die_f_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_finish()... ");
	FLAC__file_encoder_finish(encoder);
	printf("OK\n");

	printf("testing FLAC__file_encoder_delete()... ");
	FLAC__file_encoder_delete(encoder);
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

FLAC__bool test_encoders()
{
	init_metadata_blocks_();

	if(!test_stream_encoder())
		return false;

	if(!test_seekable_stream_encoder())
		return false;

	if(!test_file_encoder())
		return false;

	(void) file_utils__remove_file(flacfilename_);
	free_metadata_blocks_();

	return true;
}
