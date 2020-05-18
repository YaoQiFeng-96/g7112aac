#include "includes.h"
#include "G7112AAC.h"

int main()
{
	CG7112AAC g7112aac(8000, 2, AV_SAMPLE_FMT_S16, AV_CODEC_ID_PCM_MULAW);

	
	FILE *infile;
	fopen_s(&infile, "8k_2_16.g711u", "rb");

	FILE *outfile;
	fopen_s(&outfile, "8k_2_16.aac", "wb");

	uint8_t frame_buffer[1024] = { 0 };
	uint8_t *aac_buffer = nullptr;
	int size = 0;

	while (!feof(infile))
	{
		int iRead = fread(frame_buffer, sizeof(uint8_t), 1024, infile);
		g7112aac.AddData(frame_buffer, iRead);

		while (g7112aac.GetData(aac_buffer, &size))
		{
			fwrite(aac_buffer, sizeof(uint8_t), size, outfile);
			free(aac_buffer);
			aac_buffer = nullptr;
			size = 0;
		}
	}

	fclose(infile);
	fclose(outfile);
	
	return 0;