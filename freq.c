#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>

#include <jack/jack.h>

struct output {
	jack_port_t *port;
	float coeffs[64];
	jack_nframes_t ncoeffs;
};

jack_port_t *input_port;
jack_client_t *client;

jack_default_audio_sample_t inbuf[64];

struct output outputs[2];
int noutputs = 0;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 */
int
process (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in, *out;
	in = jack_port_get_buffer (input_port, nframes);
	for(int k=0; k<noutputs; k++) {
		out = jack_port_get_buffer (outputs[k].port, nframes);

		for(jack_nframes_t i=0; i<nframes; i++) {
			out[i] = .0f;
			for(jack_nframes_t j=0; j<outputs[k].ncoeffs; j++) {
				jack_default_audio_sample_t sample;
				if(i>=j)
					sample = in[i-j];
				else
					sample = inbuf[64+i-j];
				out[i] += outputs[k].coeffs[j] * sample;
			}
		}
	}

	for(jack_nframes_t j=0; j<64; j++)
		inbuf[j] = in[nframes-64+j];

	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
	exit (1);
}

void load(char *str) {
	const char seps[] = " ";
	char *token;
	char name[16];

	token = strtok(str, seps);
	sscanf(token, "%s", name);

	outputs[noutputs].port = jack_port_register (client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	token = strtok(NULL, seps);
	outputs[noutputs].ncoeffs = 0;
	while(token != NULL) {
		outputs[noutputs].coeffs[outputs[noutputs].ncoeffs] = atof(token);
		outputs[noutputs].ncoeffs++;
		token = strtok(NULL, seps);
	}
	noutputs++;
}

int
main (int argc, char *argv[])
{
	const char *client_name = "freq";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate.
	 */

	printf ("engine sample rate: %" PRIu32 "\n",
		jack_get_sample_rate (client));

	input_port = jack_port_register (client, "input",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsInput, 0);

	if (input_port == NULL) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}

	load(strdup("out_low -0.0028137 -0.0052045 -0.0079595 -0.0026899 0.0220916 0.0709401 0.1337142 0.1875272 0.2087891 0.1875272 0.1337142 0.0709401 0.0220916 -0.0026899 -0.0079595 -0.0052045 -0.0028137"));
	load(strdup("out_high 0.0027881 0.0051571 0.0078869 0.0026654 -0.0218902 -0.0702935 -0.1324954 -0.1858178 0.7908436 -0.1858178 -0.1324954 -0.0702935 -0.0218902 0.0026654 0.0078869 0.0051571 0.0027881"));

	for(int k=0; k<noutputs; k++) {
		printf("%d.coeffs\t", k);
		for(int i=0; i<outputs[k].ncoeffs; i++)
			printf("%f ", outputs[k].coeffs[i]);
		putchar('\n');
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	/* keep running until stopped by the user */

	fgetc( stdin );

	jack_client_close (client);
	exit (0);
}
