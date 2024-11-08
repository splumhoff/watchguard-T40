/*!
 * \brief wrapper to gunzip one file to another.
 *
 * Copyright 2011 WatchGuard Technologies Inc. 
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "zlib.h"

/* 2 ** 20 bytes, nothing magic here.  must be smaller than
 * biggest ssize_t. */
#define WG_GZ_BUFLEN 1048576

/*!
 * \func wg_gunzip
 * \brief Unpack a gzipped input file into an output file.
 * \param infile    [IN]    path to input file
 * \param outfile   [IN]    path to output file
 *
 * infile = path to input file
 * outfile = path to output file or NULL for standard output
 *
 * Return: 0 = OK, -1 = error
 * May write error message to standard error
 */

int wg_gunzip(const char const * infile, const char const *outfile)
{
	int ifd = -1;               /* input file descriptor  */
	int ofd = -1;               /* output file descriptor */
	void * buffer = NULL;       /* read buffer            */
	const char * pickup = NULL; /* pickup from buffer     */
	gzFile input = NULL;        /* input handle           */
	ssize_t readlen = 0;        /* bytes read             */
	ssize_t writelen = 0;       /* bytes written or -1    */
	int result = -1;            /* main() result          */
	int errnum = 0;             /* error number           */
	const char * errmsg = NULL; /* error message          */

	if (infile == NULL) {
		errno = EINVAL;
		goto outtahere;
	}


	buffer = malloc(WG_GZ_BUFLEN);
	if (buffer == NULL) {
		errmsg = strerror(errno);
		if (errmsg == NULL) errmsg = "(unknown error)";
		fprintf(stderr, "Error allocating input buffer: %s\n", errmsg);
		goto outtahere;
	}

	ifd = open(infile, O_RDONLY);
	if (ifd == -1) {
		errmsg = strerror(errno);
		if (errmsg == NULL) errmsg = "(unknown error)";
		fprintf(stderr, "Error opening %s: %s\n", infile, errmsg);
		goto outtahere;
	}

	if (outfile == NULL) {
		ofd = fileno(stdout);
	}
	else {
		ofd = open(outfile, O_WRONLY);
		if (ofd == -1) {
			errmsg = strerror(errno);
			if (errmsg == NULL) errmsg = "(unknown error)";
			fprintf(stderr, "Error opening %s: %s\n", outfile, errmsg);
			goto outtahere;
		}
	}

	input = gzdopen(ifd, "rb");
	if (input == NULL) {
		errmsg = gzerror(input, &errnum);
		if (errnum == Z_ERRNO) {
			errmsg = strerror(errno);
		}
		if (errmsg == NULL) errmsg = "(unknown error)";
		fprintf(stderr, "gzdopen %s failed: %s\n", infile, errmsg);
		goto outtahere;
	}

	for (;;) {
		readlen = gzread(input, buffer, WG_GZ_BUFLEN);
		if (readlen == -1) {
			errmsg = gzerror(input, &errnum);
			if (errnum == Z_ERRNO) {
				errmsg = strerror(errno);
			}
			if (errmsg == NULL) errmsg = "(unknown error)";
			fprintf(stderr, "read %s failed: %s\n", infile, errmsg);
			goto outtahere;
		}
		if (readlen == 0) {
			result = 0;
			break;
		}
		pickup = (const char *)buffer;
		while (readlen > 0) {
			writelen = write(ofd, pickup, readlen);
			if (writelen == -1) {
				errmsg = strerror(errno);
				if (errmsg == NULL) errmsg = "(unknown error)";
				fprintf(stderr, "write %s failed: %s\n", outfile, errmsg);
				goto outtahere;
			}
			readlen -= writelen;
			pickup += writelen;
		}
	}

outtahere:

	if (input != NULL) {
		gzclose(input); /* also closes ifd */
	}

	if (ofd != -1 && ofd != fileno(stdout)) {
		close(ofd);
	}

	if (buffer != NULL) {
		free(buffer);
	}

	return result;
}

#ifdef WG_TEST_STUB

/*!
 * \brief Test stub for wg_gunzip().
 * \param argc    [IN]    Argument count
 * \param argv    [IN]    Argument vector
 *
 * \return 0 on success, <0> otherwise
 *
 * Usage: <name> infile [outfile]
 *
 * Expects gzipped inputfile which it unpacks to optional
 * outfile, standard output if not given.
 *
 * Compile with -DWG_TEST_STUB to enable this mainline.
 */
int main(int argc, char **argv)
{
	const char *myname = NULL;
	int result = -1;
	const char *infile = NULL;
	const char *outfile = NULL;

	for (myname = argv[0]; *myname; ++myname) ;
	for (; myname != argv[0] && *myname != '/'; --myname);
	++myname;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s infile [outfile]\n", myname);
		goto outtahere;
	}

	infile = argv[1];

	if (argc > 2) {
		outfile = argv[2];
	}

	result = wg_gunzip(infile, outfile);

outtahere:
	return result;
}
#endif
