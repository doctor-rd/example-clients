#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>

#include <jack/jack.h>

jack_port_t *input_port;
jack_port_t *output_port[2];
jack_client_t *client;

jack_default_audio_sample_t inbuf[64];

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 */
int
process (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in, *out[2];
	in = jack_port_get_buffer (input_port, nframes);
	for(int i=0; i<2; i++)
		out[i] = jack_port_get_buffer (output_port[i], nframes);

	for(jack_nframes_t i=0; i<nframes; i++) {
		out[0][i] = .0f;
		for(jack_nframes_t j=0; j<10; j++) {
			jack_default_audio_sample_t sample;
			if(i>=j)
				sample = in[i-j];
			else
				sample = inbuf[64+i-j];
			out[0][i] += .1f * sample;
		}
		out[1][i] = in[i] - out[0][i];
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
	output_port[0] = jack_port_register (client, "out_low", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	output_port[1] = jack_port_register (client, "out_high", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	if ((input_port == NULL) || (output_port[0] == NULL || output_port[1] == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
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
