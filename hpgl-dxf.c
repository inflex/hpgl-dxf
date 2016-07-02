/**
 * hpgl-dxf
 *
 * Written and maintained by Paul L Daniels [ pldaniels at pldaniels com ]
 *
 * Convert HPGL files into simple single-layer DXF files
 *
 * First version written: Monday July 02, 2007
 *
 * Original purpose of this program was to allow me to convert HPGL files 
 * generated by EaglePCB into DXF files that I could send to my laser cutting
 * service
 *
 **/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>


#define HPGLDXF_VERSION "0.0.2"
#define HPGLDXF_DEFAULT_INIT_STRING "0\nSECTION\n0\nENTITIES\n0\n"
#define HPGLDXF_DEFAULT_END_STRING "ENDSEC\n"

#define PEN_UP 0
#define PEN_DOWN 1

char HPGLDXF_HELP[]="hpgl-dxf: HPGL to DXF converter (for simple HPGL)\n"
"Written by Paul L Daniels.\n"
"Distributed under the BSD Revised licence\n"
"This software is available at http://pldaniels.com/hpgl-dxf\n"
"\n"
"Usage: hpgl-distiller -i <input HPGL> -o <output DXF file> [-v] [-d] [-h]\n"
"\n"
"\t-i <input HPGL> : Specifies which file contains the full HPGL file that is to be distilled.\n"
"\t-o <output file> : specifies which file the distilled HPGL is to be saved to.\n"
"\n"
"\t-v : Display current software version\n"
"\t-d : Enable debugging output (verbose)\n"
"\t-h : Display this help.\n"
"\n";

struct hpgld_glb {
	int status;
	int debug;
	char *init_string;
	char *input_filename;
	char *output_filename;
};

struct pen {
	int status;
	double x, y;
};

/**
 * HPGLDXF_show_version
 *
 * Display the current HPGL-Distiller version
 */
int HPGLDXF_show_version( struct hpgld_glb *glb ) {
	fprintf(stderr,"%s\n", HPGLDXF_VERSION);
	return 0;
}

/**
 * HPGLDXF_show_help
 *
 * Display the help data for this program
 */
int HPGLDXF_show_help( struct hpgld_glb *glb ) {
	HPGLDXF_show_version(glb);
	fprintf(stderr,"%s\n", HPGLDXF_HELP);

	return 0;

}




/**
 * Actually do something with the commands that we've filtered from the input
 * HPGL file
 *
 * struct hpgld_glb *glb: global program data
 * FILE *f: file to send the output to (if any, namely on a PA or PR command)
 * struct pen *p: structure containing the global pen information
 * char *command: the command that we received from the input HPGL file
 *
 *
 */
int HPGLDXF_process_command(struct hpgld_glb *glb, FILE *f, struct pen *p, char *command ) {
	char *cp = command;
	char *ep;
	double px,py;

	switch (command[1]) {
		case 'U':
		case 'u':
			p->status=PEN_UP;
			break;

		case 'D':
		case 'd':
			p->status=PEN_DOWN;
			break;

		case 'A':
		case 'a':
		case 'r':
		case 'R':

			/** decode our X and Y coords from the PA command **/
			while ((*cp != '\0')&&(!isdigit(*cp))&&(*cp != '-')) cp++;

			if (*cp == '\0') {
				fprintf(stderr,"ERROR: Cannot find coordinates in '%s'\n", command);
				break;
			}

			ep = strchr(cp,','); // find the end of the current number
			if (!ep) {
				fprintf(stderr,"ERROR: Cannot find coordinate seperator in '%s'\n",command);
				break;
			}

			*ep = '\0';
			px = strtod(cp,NULL);
			py = strtod(ep+1,NULL);
			*ep = ',';
			if (glb->debug) fprintf(stdout,"Converted '%s' to '%0.3f, %0.3f'\n", command, px, py);

			if ((command[1] == 'r')||(command[1] =='R')) {
				/** Relative draw **/
				px = p->x +px;
				py = p->y +py;
			}


			if (p->status == PEN_DOWN) {
				fprintf(f,"LINE\n"
						"10\n"
						"%0.3f\n"
						"20\n"
						"%0.3f\n"
						"11\n"
						"%0.3f\n"
						"21\n"
						"%0.3f\n"
						"0\n"
						, p->x
						, p->y
						, px
						, py
						);
			}

			p->x = px;
			p->y = py;

			break;

		default:
			fprintf(stderr,"ERROR: Don't know how to handle command '%s'\n", command);
	}


	return 0;
}

/**
 * HPGLDXF_init
 *
 * Initializes any variables or such as required by
 * the program.
 */
int HPGLDXF_init( struct hpgld_glb *glb )
{
	glb->status = 0;
	glb->debug = 0;
	glb->init_string = HPGLDXF_DEFAULT_INIT_STRING;
	glb->input_filename = NULL;
	glb->output_filename = NULL;

	return 0;
}

/**
 * HPGLDXF_parse_parameters
 *
 * Parses the command line parameters and sets the
 * various HPGL-Distiller settings accordingly
 */
int HPGLDXF_parse_parameters( int argc, char **argv, struct hpgld_glb *glb )
{

	char c;

	do {
		c = getopt(argc, argv, "I:i:o:dvh");
		switch (c) { 
			case EOF: /* finished */
				break;

			case 'i':
				glb->input_filename = strdup(optarg);
				break;

			case 'o':
				glb->output_filename = strdup(optarg);
				break;

			case 'd':
				glb->debug = 1;
				break;

			case 'h':
				HPGLDXF_show_help(glb);
				exit(1);
				break;

			case 'v':
				HPGLDXF_show_version(glb);
				exit(1);
				break;

			case '?':
				break;

			default:
				fprintf(stderr, "internal error with getopt\n");
				exit(1);

		} /* switch */
	} while (c != EOF); /* do */
	return 0;
}



/**
 * HPGL-DXF,  main body
 *
 * This program inputs an existing HPGL file which
 * contains a drawing that we'd like to convert into 
 * DXF format.
 *
 *
 */
int main(int argc, char **argv) {

	struct hpgld_glb glb;
	FILE *fi, *fo;
	char *data, *p;
	struct stat st;
	int stat_result;
	size_t file_size;
	size_t read_size;
	struct pen pen;


	pen.x = 0;
	pen.y = 0;
	pen.status = PEN_UP;

	/* Initialize our global data structure */
	HPGLDXF_init(&glb);


	/* Decyper the command line parameters provided */
	HPGLDXF_parse_parameters(argc, argv, &glb);

	/* Check the fundamental sanity of the variables in
		the global data structure to ensure that we can
		actually perform some meaningful work
		*/
	if ((glb.input_filename == NULL)) {
		fprintf(stderr,"Error: Input filename is NULL.\n");
		exit(1);
	}
	if ((glb.output_filename == NULL)) {
		fprintf(stderr,"Error: Output filename is NULL.\n");
		exit(1);
	}

	/* Check to see if we can get information about the
		input file. Specifically we'll be wanting the 
		file size so that we can allocate enough memory
		to read in the whole file at once (not the best
		method but it's certainly a lot easier than trying
		to read in line at a time when the file can potentially
		contain non-ASCII chars 
		*/
	stat_result = stat(glb.input_filename, &st);
	if (stat_result) {
		fprintf(stderr,"Cannot stat '%s' (%s)\n", argv[1], strerror(errno));
		return 1;
	}

	/* Get the filesize and attempt to allocate a block of memory
		to hold the entire file
		*/
	file_size = st.st_size;
	data = malloc(sizeof(char) *(file_size +1));
	if (data == NULL) {
		fprintf(stderr,"Cannot allocate enough memory to read input HPGL file '%s' of size %d bytes (%s)\n", glb.input_filename, file_size, strerror(errno));
		return 2;
	}

	/* Attempt to open the input file as read-only 
	*/
	fi = fopen(glb.input_filename,"r");
	if (!fi) {
		fprintf(stderr,"Cannot open input file '%s' for reading (%s)\n", glb.input_filename, strerror(errno));
		return 3;
	}

	/* Attempt to open the output file in write mode
		(no appending is done, any existing file will be
		overwritten
		*/
	fo = fopen(glb.output_filename,"w");
	if (!fo) {
		fprintf(stderr,"Cannot open input file '%s' for writing (%s)\n", glb.output_filename, strerror(errno));
		return 4;
	}

	/* Read in the entire input file .
		Compare the size read with the size
		of the file as a double check.
		*/
	read_size = fread(data, sizeof(char), file_size, fi);
	fclose(fi);
	if (read_size != file_size) {
		fprintf(stderr,"Error, the file size (%d bytes) and the size of the data read (%d bytes) do not match.\n", file_size, read_size);
		return 5;
	}


	/* Put a terminating '\0' at the end of the data so
		that our string tokenizing functions won't overrun
		into no-mans-land and cause segfaults
		*/
	data[file_size] = '\0';

	/* Generate any preliminary intializations to the output
		file first.
		*/
	fprintf(fo,"%s", glb.init_string);

	/* Go through the entire data block read from the
		input file and break it off one piece at a time
		as delimeted/tokenized by ;, \r or \n characters.
		*/
	p = strtok(data, ";\n\r");
	while (p) {

		if (glb.debug) printf("in: %s  ",p);

		/* Compare the segment of data obtained
			from the tokenizing process to the list
			of 'valid' HPGL commands that we want to
			accept.  Anything not in this list we 
			simply discard.
			*/
		if (
				(strncmp(p,"PA",2)==0)	// Plot Absolute
				||(strncmp(p,"PR",2)==0)	// Pen Relative
				||(strncmp(p,"PD",2)==0)	// Pen Down
				||(strncmp(p,"PU",2)==0)	// Pen Up
			) {

			HPGLDXF_process_command(&glb, fo, &pen, p);
		} else {


			if (glb.debug) printf("ignored\n");
		} /* if strncmp... */

		/* Get the next block of data from the input data
		*/
		p = strtok(NULL,";\n\r");


	} /** while more tokenizing **/

	/* Close the output file */
	fprintf(fo,"%s",HPGLDXF_DEFAULT_END_STRING);
	fclose(fo);

	/* Release the memory we allocated to hold the input HPGL file */
	free(data);

	return 0;
}

/** END **/


