#include "G7112AAC.h"

CG7112AAC::CG7112AAC(int sample_rate, int channels, AVSampleFormat sample_fmt, AVCodecID codec_id) :
	m_sampleRate(sample_rate), m_channels(channels), m_sampleFmt(sample_fmt), m_codecId(codec_id),
	m_decoder(nullptr), m_deCtx(nullptr), m_dePkt(nullptr), m_deFrame(nullptr),
	m_initDecoder(false), m_initDecoderOK(false),
	m_encoder(nullptr), m_enCtx(nullptr), m_enPkt(nullptr), m_enFrame(nullptr),
	m_initEncoder(false), m_initEncoderOK(false), m_resampleCtx(nullptr), m_pts(0),
	m_pcmBuffer(nullptr), m_pcmSize(0)
{
	m_pcmBuffer = new uint8_t[PCM_BUFFER_SIZE];
	memset(m_pcmBuffer, 0, PCM_BUFFER_SIZE);

	for (auto i = 0; i < 8; i++)
	{
		m_pcmPointer[i] = new uint8_t[PCM_BUFFER_SIZE];
	}
}


CG7112AAC::~CG7112AAC()
{
	if (nullptr != m_dePkt)
	{
		av_packet_free(&m_dePkt);
		m_dePkt = nullptr;
	}
	if (nullptr != m_deFrame)
	{
		av_frame_free(&m_deFrame);
		m_deFrame = nullptr;
	}
	if (nullptr != m_deCtx)
	{
		avcodec_close(m_deCtx);
		avcodec_free_context(&m_deCtx);
		m_deCtx = nullptr;
	}
	if (nullptr != m_decoder)
		m_decoder = nullptr;

	if (nullptr != m_enPkt)
	{
		av_packet_free(&m_enPkt);
		m_enPkt = nullptr;
	}
	if (nullptr != m_enFrame)
	{
		av_frame_free(&m_enFrame);
		m_enFrame = nullptr;
	}
	if (nullptr != m_enCtx)
	{
		avcodec_close(m_enCtx);
		avcodec_free_context(&m_enCtx);
		m_enCtx = nullptr;
	}
	if (nullptr != m_encoder)
		m_encoder = nullptr;

	if (nullptr != m_resampleCtx)
	{
		swr_free(&m_resampleCtx);
		m_resampleCtx = nullptr;
	}

	if (nullptr != m_pcmBuffer)
	{
		delete[] m_pcmBuffer;
		m_pcmBuffer = nullptr;
		m_pcmSize = 0;
	}

	for (auto i = 0; i < 8; i++)
	{
		delete[] m_pcmPointer[i];
		m_pcmPointer[i] = nullptr;
	}

}


bool CG7112AAC::InitDecoder()
{
	if (m_codecId != AV_CODEC_ID_PCM_MULAW && m_codecId != AV_CODEC_ID_PCM_ALAW)
		return false;

	m_decoder = avcodec_find_decoder(m_codecId);
	if (!m_decoder)
	{
		fprintf(stderr, "decoder not found. \n");
		return false;
	}

	m_deCtx = avcodec_alloc_context3(m_decoder);
	if (!m_deCtx)
	{
		fprintf(stderr, "could not allocate audio decoder context. \n");
		return false;
	}
	m_deCtx->sample_rate = m_sampleRate;
	m_deCtx->sample_fmt = m_sampleFmt;
	m_deCtx->channels = m_channels;

	if (avcodec_open2(m_deCtx, m_decoder, NULL) < 0)
	{
		fprintf(stderr, "could not open audio decoer. \n");
		return false;
	}

	m_dePkt = av_packet_alloc();
	if (nullptr == m_dePkt)
	{
		return false;
	}
	m_deFrame = av_frame_alloc();
	if (nullptr == m_deFrame)
	{
		return false;
	}
	return true;
}


void CG7112AAC::AddData(uint8_t * pData, int iSize)
{
	if (!m_initDecoder)
	{
		m_initDecoder = true;
		m_initDecoderOK = InitDecoder();
	}
	if (!m_initDecoderOK)
		return;

	m_dePkt->size = iSize;
	m_dePkt->data = (uint8_t *)av_malloc(m_dePkt->size);
	memcpy(m_dePkt->data, pData, m_dePkt->size);
	int ret = av_packet_from_data(m_dePkt, m_dePkt->data, m_dePkt->size);
	if (ret < 0)
	{
		fprintf(stderr, "av_packet_from_data error. \n");
		av_free(m_dePkt->data);
		m_dePkt->data = nullptr;
		m_dePkt->size = 0;
		return;
	}

	ret = avcodec_send_packet(m_deCtx, m_dePkt);
	av_packet_unref(m_dePkt);
	if (ret < 0)
	{
		fprintf(stderr, "av_codec_send_packet error. \n");
		return;
	}
	ret = avcodec_receive_frame(m_deCtx, m_deFrame);
	if (ret < 0)
	{
		fprintf(stderr, "avcodec_receive_frame error. \n");
		return;
	}

	int data_size = av_get_bytes_per_sample(m_deCtx->sample_fmt);
	if (data_size < 0)
	{
		fprintf(stderr, "failed to calculate audio data size. \n");
		return;
	}
	memcpy(m_pcmBuffer + m_pcmSize, m_deFrame->data[0], m_deFrame->nb_samples*m_deCtx->channels*data_size);
	m_pcmSize += m_deFrame->nb_samples*m_deCtx->channels*data_size;
}


bool CG7112AAC::InitEncoder()
{
	m_encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!m_encoder)
	{
		fprintf(stderr, "encoder not found. \n");
		return false;
	}
	m_enCtx = avcodec_alloc_context3(m_encoder);
	if (!m_enCtx)
	{
		fprintf(stderr, "could not allocate audio encoder context. \n");
		return false;
	}
	m_enCtx->channels = m_channels;
	m_enCtx->channel_layout = av_get_default_channel_layout(m_channels);
	m_enCtx->sample_rate = m_sampleRate;
	m_enCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	m_enCtx->bit_rate = 64000;
	m_enCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	if (avcodec_open2(m_enCtx, m_encoder, NULL) < 0)
	{
		fprintf(stderr, "could not open encoder. \n");
		return false;
	}
	m_enPkt = av_packet_alloc();
	if (nullptr == m_enPkt)
	{
		return false;
	}
	m_enFrame = av_frame_alloc();
	if (nullptr == m_enFrame)
	{
		return false;
	}
	m_resampleCtx = swr_alloc_set_opts(NULL, m_enCtx->channel_layout, m_enCtx->sample_fmt,
		m_enCtx->sample_rate, m_enCtx->channel_layout, m_sampleFmt, m_enCtx->sample_rate, 0, NULL);
	if (nullptr == m_resampleCtx)
	{
		fprintf(stderr, "could not allocate audio sample context. \n");
		return false;
	}
	if (swr_init(m_resampleCtx) < 0)
	{
		fprintf(stderr, "could not open resample context. \n");
		return false;
	}
	return true;
}


bool CG7112AAC::GetData(uint8_t *& pData, int * iSize)
{
	if (false == m_initEncoder)
	{
		m_initEncoder = true;
		m_initEncoderOK = InitEncoder();
	}
	if (false == m_initEncoderOK)
		return false;

	int data_size = av_get_bytes_per_sample(m_sampleFmt);
	if (m_pcmSize <= data_size * 1024 * m_channels)
		return false;

	memcpy(m_pcmPointer[0], m_pcmBuffer, data_size * 1024 * m_channels);
	m_pcmSize = m_pcmSize - data_size * 1024 * m_channels;
	memcpy(m_pcmBuffer, m_pcmBuffer + data_size * 1024 * m_channels, m_pcmSize);

	m_enFrame->pts = m_pts;
	m_pts += 1024;
	m_enFrame->nb_samples = 1024;
	m_enFrame->format = m_enCtx->sample_fmt;
	m_enFrame->channel_layout = m_enCtx->channel_layout;
	m_enFrame->sample_rate = m_enCtx->sample_rate;
	if (av_frame_get_buffer(m_enFrame, 0) < 0)
	{
		fprintf(stderr, "could not allocate audio data buffer. \n");
		return false;
	}
	if (swr_convert(m_resampleCtx, m_enFrame->extended_data, m_enFrame->nb_samples,
		(const uint8_t **)m_pcmPointer, 1024) < 0)
	{
		fprintf(stderr, "could not convert input sample. \n");
		if (nullptr != m_enFrame)
			av_frame_unref(m_enFrame);
		return false;
	}
	int ret = avcodec_send_frame(m_enCtx, m_enFrame);
	if (ret < 0)
	{
		fprintf(stderr, "avcodec_send_frame error.\n");
		if (nullptr != m_enFrame)
			av_frame_unref(m_enFrame);
		return false;
	}
	if (nullptr != m_enFrame)
		av_frame_unref(m_enFrame);
	
	ret = avcodec_receive_packet(m_enCtx, m_enPkt);
	if (ret < 0)
	{
		fprintf(stderr, "avcodec_receive_packet error. \n");
		return false;
	}

	pData = (uint8_t *)malloc(sizeof(uint8_t)*(m_enPkt->size + 7));
	*iSize = m_enPkt->size + 7;
	AddADTS(pData, m_enPkt->size + 7);
	memcpy(pData + 7, m_enPkt->data, m_enPkt->size);
	av_packet_unref(m_enPkt);
	return true;
}


void CG7112AAC::AddADTS(uint8_t* &pData, int packLen)
{
	int profile = 1; // AAC LC  
	int freqIdx = 0xb; // 44.1KHz  
	int chanCfg = m_channels; // CPE  

	if (96000 == m_sampleRate)
	{
		freqIdx = 0x00;
	}
	else if (88200 == m_sampleRate)
	{
		freqIdx = 0x01;
	}
	else if (64000 == m_sampleRate)
	{
		freqIdx = 0x02;
	}
	else if (48000 == m_sampleRate)
	{
		freqIdx = 0x03;
	}
	else if (44100 == m_sampleRate)
	{
		freqIdx = 0x04;
	}
	else if (32000 == m_sampleRate)
	{
		freqIdx = 0x05;
	}
	else if (24000 == m_sampleRate)
	{
		freqIdx = 0x06;
	}
	else if (22050 == m_sampleRate)
	{
		freqIdx = 0x07;
	}
	else if (16000 == m_sampleRate)
	{
		freqIdx = 0x08;
	}
	else if (12000 == m_sampleRate)
	{
		freqIdx = 0x09;
	}
	else if (11025 == m_sampleRate)
	{
		freqIdx = 0x0a;
	}
	else if (8000 == m_sampleRate)
	{
		freqIdx = 0x0b;
	}
	else if (7350 == m_sampleRate)
	{
		freqIdx = 0xc;
	}
	// fill in ADTS data  
	pData[0] = 0xFF;
	pData[1] = 0xF1;
	pData[2] = ((profile) << 6) + (freqIdx << 2) + (chanCfg >> 2);
	pData[3] = (((chanCfg & 3) << 6) + (packLen >> 11));
	pData[4] = ((packLen & 0x7FF) >> 3);
	pData[5] = (((packLen & 7) << 5) + 0x1F);
	pData[6] = 0xFC;
}
