/**
 * @file vmsbackup.c
 */

/**
 *
 * @mainpage
 * @version Feb_2024
 * @par Read Me
 *
 *  Title:
 *	vmsbackup
 *
 *  Decription:
 *	Program to read and decode VMS Backup images
 *
 *  @author John Douglas CAREY.
 *  @author Sven-Ove Westberg    (version 3.0 and up)
 *  @author Dave Shepperd, (DMS) vmsbackup@dshepperd.com
 *  		(version Mar_2003 with snipits from Sven-Ove's
 *  		version 4-1-1)
 *
 *  https://github.com/DaveShepperd/vmsbackup
 *
 *  Net-addess (as of 1986 or so; it is highly unlikely these still work):
 *	john%monu1.oz@seismo.ARPA
 *	luthcad!sow@enea.UUCP
 *  vmsbackup@dshepperd.com
 *
 *  History:
 *	Version 1.0 - September 1984
 *		Can only read variable length records
 *	Version 1.1
 *		Cleaned up the program from the original hack
 *		Can now read stream files
 *	Version 1.2
 *		Now convert filename from VMS to UNIX
 *			and creates sub-directories
 *	Version 1.3
 *		Works on the Pyramid if SWAP is defined
 *	Version 1.4
 *		Reads files spanning multiple tape blocks
 *	Version 1.5
 *		Always reset reclen = 0 on file open
 *		Now output fixed length records
 *
 *      Version 2.0 - July 1985
 *		VMS Version 4.0 causes a rethink !!
 *		Now use mtio operations instead of opening and closing file
 *		Blocksize now grabed from the label
 *
 *	Version 2.1 - September 1985
 *		Handle variable length records of zero length.
 *
 *	Version 2.2 - July 1986
 *		Handle FORTRAN records of zero length.
 *		Inserted exit(0) at end of program.
 *		Distributed program in aus.sources
 *
 *	Version 2.3 - August 1986
 *		Handle FORTRAN records with record length fields
 *		at the end of a block
 *		Put debug output to a file.
 *		Distributed program in net.sources
 *
 *	Version 3.0 - December 1986
 *		Handle multiple saveset 
 *		Remote tape
 *		Interactive mode
 *		File name selection with meta-characters
 *		Convert ; to : in VMS filenames
 *		Flag for usage of VMS directory structure
 *		Flag for "useless" files  eg. *.exe
 *		Flag for use VMS version in file names
 *		Flag for verbose mode
 *		Flag to list the contents of the tape
 *		Distributed to mod.sources
 *
 *	Version 3.1 - March 2003 (DMS)
 *		ANSI'fy the source
 *		added option, -i, to read "tape file" from disk
 *		added option, -n, to read specific saveset from disk/tape
 *		changed option, -v, to accept a bit mask to enable various squawks
 *		Optimise the writing of data (avoid doing single char output where possible)
 *		Added handling of redundancy groups and duplicate and missing blocks.
 *		Fixed numerous crashes caused by various mis-decodes
 *		Record and decode errors are no longer fatal.
 *		Cut and pasted select features from the 4-1-1 release
 *		Added vms2unix date function and utime()'d extracted files.
 *			(NOTE: There may be a problem with this since I
 *			found several savesets made by VMS versions 3.x and 4.x
 *			where the date seemed to be ~4 years too young. Newer
 *			savesets [> VMS 5.0?] seem to have dates set correctly;
 *			Was there a change in the VMS epoch at some point?).
 *		Always walk the file data chain even when only doing a directory
 *			listing in order to identify those files which may present
 *			decode or other errors.
 *		Due to bizzare data in the VAR records found in .mai and .dir
 *			files, force it never to extract them or even walk the chain.
 *		Recover from BACKUP's occasional mistake of leaving off the
 *			record length bytes from record 0 of vfc files.
 *		Added some 'doxygen'ated (but untested) comments.
 *
 *  Version 3.2 - April 2020 (DMS)
 *  	Added -I, to read SIMH format 'tape file' from disk
 *
 *  Verson 3.3 - Janurary 2022 (DMS)
 *      Fixed comments. Added a -S to skip to HDR1 record.
 *
 *  Version 3.4 - November 2023 (DMS)
 *  	Added -R to make it not lowercase filenames and to strip
 *  	off the file version number completely and only output
 *  	the latest version of any file.
 *
 *  Version 3.5 - January 2024 (DMS)
 *  	Added -L to provide lowercase directory and filenames.
 *
 *  Version 3.6 - January 2024 (DMS)
 *  	Added -E, reworked the -e option a bit.
 *  	Fixed some comments.
 *
 *  Version 3.7 - Feburary 2024 (DMS)
 *  	Added support for optional VFC decode in output.
 *  	Changed the way it handles record delimiters.
 *
 *  Version 3.8 - Feburary 2024 (DMS)
 *  	Added support for fixed length input records.
 *  	Added support for getopt_long().
 *  	Added a --record option.
 *  	Added rfm/size/att to directory listing.
 *  	Write binary output and filename if record errors
 *  	detected.
 *  	Updated help message.
 *  	See README.md or vmsbackup.html for further details.
 *
 *  Versin 3.9 - April 2024 (DMS)
 *      Added support for building under MSYS2 on Windows
 *
 *  Versin 3.10 - April 2024 (DMS)
 *  	Fixed MSYS2 builds and added ones for Linux, MinGW32 and
 *  	PiOS (32 bit only).
 *
 *  Versin 3.11 - April 2024 (DMS)
 *  	Fixed MSYS2 and MinGW builds. Needed the O_BINARY and
 *  	"b" flags.
 *
 *  Versin 3.12 - May 2024 (DMS)
 *  	Re-worked the method of exporting binary as a result of
 *  	errors in VAR/VFC record structures.
 *
 *  Installation:
 *
 *	Computer Centre
 *	Monash University
 *	Wellington Road
 *	Clayton
 *	Victoria	3168
 *	AUSTRALIA
 *
 */

#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<strings.h>
#include	<stdlib.h>
#include	<stddef.h>
#include	<unistd.h>
#include	<getopt.h>
#include	<time.h>
#include	<utime.h>
#include	<errno.h>
#include	<sys/types.h>
#ifdef REMOTE
	#include	<local/rmt.h>
#endif
#include	<sys/stat.h>
#if HAVE_MTIO
#include	<sys/ioctl.h>
#include	<sys/mtio.h>
#endif
#include	<sys/file.h>

#include	"libvmsbackup.h"

#if MSYS2 || MINGW
#define MKDIR(a,b) mkdir(a)
#define OPEN_FLAGS O_RDONLY|O_BINARY
#else
#define MKDIR(a,b) mkdir(a,b)
#define OPEN_FLAGS O_RDONLY
#endif

#ifndef n_elts
#define n_elts(x) (int)(sizeof(x)/sizeof((x)[0]))
#endif

extern int match ( const char *string, const char *pattern );
static int typecmp ( const char *str, int which );
static void usage ( const char *progname, int details );
static unsigned long get_block_number( struct vmb_ctx *ctx, unsigned char *bptr );

#if defined(__GNUC__) || defined(__clang__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#define MAX_FILENAME_LEN (128)
#define MAX_FORMAT_LEN	 (16)

struct bbh
{
	short bbh_dol_w_size;
	short bbh_dol_w_opsys;
	short bbh_dol_w_subsys;
	short bbh_dol_w_applic;
	long bbh_dol_l_number;
	char bbh_dol_t_spare_1[20];
	short bbh_dol_w_struclev;
	short bbh_dol_w_volnum;
	long bbh_dol_l_crc;
	long bbh_dol_l_blocksize;
	long bbh_dol_l_flags;
	char bbh_dol_t_ssname[32];
	short bbh_dol_w_fid[3];
	short bbh_dol_w_did[3];
	char bbh_dol_t_filename[128];
	char bbh_dol_b_rtype;
	char bbh_dol_b_rattrib;
	short bbh_dol_w_rsize;
	char bbh_dol_b_bktsize;
	char bbh_dol_b_vfcsize;
	short bbh_dol_w_maxrec;
	long bbh_dol_l_filesize;
	char bbh_dol_t_spare_2[22];
	short bbh_dol_w_checksum;
};

struct brh
{
	short brh_dol_w_rsize;
	short brh_dol_w_rtype;
	long brh_dol_l_flags;
	long brh_dol_l_address;
	long brh_dol_l_spare;
};

/* define record types */

#define	brh_dol_k_null	0
#define	brh_dol_k_summary	1
#define	brh_dol_k_volume	2
#define	brh_dol_k_file	3
#define	brh_dol_k_vbn	4
#define brh_dol_k_physvol	5
#define brh_dol_k_lbn	6
#define	brh_dol_k_fid	7

struct bsa
{
	short bsa_dol_w_size;
	short bsa_dol_w_type;
	char bsa_dol_t_text[1];
};

typedef enum
{
	GET_IDLE,
	GET_RCD_COUNT,
	GET_VFC,
	GET_DATA
} FileState_t;

typedef enum
{
	 OPT_UNDEFINED
	,OPT_DVD			/* -i */
	,OPT_EXTRACT		/* --extract */
	,OPT_FILE			/* -f */
	,OPT_HDR1_NUMBER	/* -s */
	,OPT_LIST			/* -t */
	,OPT_SET_NAME		/* -n */
	,OPT_SIMH			/* -I */
	,OPT_VER_DELIMIT	/* --delimiter */
	,OPT_VERBOSE		/* -v */
	,OPT_PROMPT			/* -w */
	,OPT_NO_VERSION		/* -R */
	,OPT_HIERARCHY		/* -d */
	,OPT_BINARY			/* -B */
	,OPT_FFRM			/* -E */
	,OPT_LOWERCASE		/* -l */
	,OPT_RECFMT			/* -F */
	,OPT_HELP			/* -h */
} Options_t;

static struct option long_options[] =
{
	{ "dvd",		no_argument,		0, OPT_DVD },
	{ "extract",	required_argument,	0, OPT_EXTRACT },
	{ "file",		required_argument,	0, OPT_FILE },
	{ "hdr1",		required_argument,	0, OPT_HDR1_NUMBER },
	{ "list",		no_argument,		0, OPT_LIST },
	{ "setname",	required_argument,	0, OPT_SET_NAME },
	{ "simh",		no_argument,		0, OPT_SIMH },
	{ "delimiter",	optional_argument,	0, OPT_VER_DELIMIT },
	{ "verbose",	required_argument,	0, OPT_VERBOSE },
	{ "prompt",		no_argument,		0, OPT_PROMPT },
	{ "noversions",	no_argument,		0, OPT_NO_VERSION },
	{ "lowercase",	no_argument,		0, OPT_LOWERCASE },
	{ "hierarchy",	no_argument,		0, OPT_HIERARCHY },
	{ "binary",		no_argument,		0, OPT_BINARY },
	{ "ffrm",		no_argument,		0, OPT_FFRM },
	{ "record",		required_argument,	0, OPT_RECFMT },
	{ "help",		no_argument,		0, OPT_HELP },
	{ 0, 0, 0, 0 }
};

struct file_details
{
	time_t ctime;
	time_t mtime;
	time_t atime;
	time_t btime;
	FILE *extf;
	FILE *altf;
	int directory;
	unsigned int size;
	int nblk;
	int lnch;
	int allocation;
	int usr;
	int grp;
	int recfmt;
	int savRecFmt;
	int recatt;
	unsigned int inboundIndex;		/* Index into specific byte in input file */
	unsigned int outboundIndex;		/* Index into specific byte in output file */
	unsigned int altboundIndex;
	int rec_count;
	int rec_padding;
	int vfcsize;					/* number of VFC bytes */
	char name[MAX_FILENAME_LEN+4];	/* Name from tape image */
	char ufname[MAX_FILENAME_LEN+MAX_FORMAT_LEN+4]; /* Name converted to Unix */
	char altUPfName[MAX_FILENAME_LEN+MAX_FORMAT_LEN+4]; /* Name converted to Unix */
	char *altUfNameOnly;			/* Place in altUPfName of '.' character at head of alternate filename */
	char *versionPtr;
	unsigned short reclen;			/* length of most recent record read */
	short fix;
	unsigned short recsize;			/* record length in FIXED and max record length in VAR and VFC formats */
	int do_vfc;
	unsigned char vfc0, vfc1;
	int do_rat;
	int do_binary;
	int errorIndex;
	int altErrorIndex;
	int file_record_error;
	int file_blk_error;
	int file_size_error;
	int file_format_error;
	FileState_t file_state;
};

#define	FAB_dol_C_RAW	0	/* undefined */
#define	FAB_dol_C_FIX	1	/* fixed-length record */
#define	FAB_dol_C_VAR	2	/* variable-length record */
#define	FAB_dol_C_VFC	3	/* variable-length with fixed-length control record */
#define FAB_dol_C_STM	4	/* RMS-11 stream record (valid only for sequential org) */
#define	FAB_dol_C_STMLF	5	/* stream record delimited by LF (sequential org only) */
#define FAB_dol_C_STMCR	6	/* stream record delimited by CR (sequential org only) */
#define FAB_dol_C_FIX11 11	/* alternate fixed-length */
#define	FAB_dol_C_MAXRFM 11	/* maximum rfm supported */
#define FAB_dol_M_MAIL (0x20)	/* this bit is set on .mai files for some reason */

#define	FAB_dol_V_FTN	0	/* FORTRAN carriage control character */
#define	FAB_dol_V_CR	1	/* line feed - record -carriage return */
#define	FAB_dol_V_PRN	2	/* print-file carriage control */
#define	FAB_dol_V_BLK	3	/* records don't cross block boundaries */

/* File record ID's */

#define FREC_END	(0x00)	/* End of list */
#define FREC_FNAME	(0x2a)	/* Filename */
#define FREC_UNK2b	(0x2b)	/* ? */
#define FREC_UNK2c	(0x2c)	/* ? */
#define FREC_UNK2d	(0x2d)	/* ? */
#define FREC_UNK2e	(0x2e)	/* ? */
#define FREC_UID	(0x2f)	/* UID */
#define FREC_UNK30	(0x30)	/* ? */
#define FREC_UNK31	(0x31)	/* ? */
#define FREC_UNK32	(0x32)	/* ? */
#define FREC_UNK33	(0x33)	/* ? */
#define FREC_FORMAT	(0x34)	/* Record format and stuff */
#define FREC_UNK35	(0x35)	/* ? */
#define FREC_CTIME	(0x36)	/* Creation time */
#define FREC_MTIME	(0x37)	/* Modification time */
#define FREC_ATIME	(0x38)	/* Access time */
#define FREC_BTIME	(0x39)	/* Backup time */
#define FREC_UNK47	(0x47)	/* ? */
#define FREC_UNK48	(0x48)	/* ? */
#define FREC_DIRECTORY	(0x49)	/* Directory file */
#define FREC_UNK4a	(0x4A)	/* ? */
#define FREC_UNK4b	(0x4B)	/* ? */
#define FREC_UNK4e	(0x4e)	/* ? */
#define FREC_UNK4f	(0x4F)	/* ? */
#define FREC_UNK50	(0x50)	/* ? */
#define FREC_UNK57	(0x57)	/* ? */

/* Summary record ID's */

#define SUMM_END	(0)	/* end of summary list */
#define SUMM_SSNAME	(1)	/* saveset name */
#define SUMM_CMDLINE	(2)	/* command line */
#define SUMM_COMMENT	(3)	/* comment */
#define SUMM_USER	(4)	/* username of creator of ss */
#define SUMM_UID	(5)	/* PID/GID of creator */
#define SUMM_CTIME	(6)	/* creation time */
#define SUMM_OSCODE	(7)	/* OS code */
#define SUMM_OSCODE_VAX (0x400) /* VAX */
#define SUMM_OSCODE_AXP (0x800) /* Alpha */
#define SUMM_OSVERSION	(8)	/* OS version */
#define SUMM_NODENAME	(9)	/* nodename */
#define SUMM_PID	(10)	/* Processor ID */
#define SUMM_DEVICE	(11)	/* device used to write ss */
#define SUMM_BCKVERSION	(12)	/* BACKUP version */
#define SUMM_BLOCKSIZE	(13)	/* /BLOCKSIZE */
#define SUMM_GROUPSIZE	(14)	/* /GROUP */
#define SUMM_BUFFCOUNT	(15)	/* /BUFFER */

#define SKIP_TO_FILE	(1)	/*!< Skip to next file */
#define SKIP_TO_BLOCK	(2)	/*!< Skip to next block */
#define SKIP_TO_SAVESET	(4)	/*!< Skip to next saveset */

#define NXT_BLK_OK	(0)	/*!< block is ok to decode */
#define NXT_BLK_EOT	(1)	/*!< we're at EOT */
#define NXT_BLK_TM	(2)	/*!< we're at a TM */
#define NXT_BLK_NOLEAD	(3)	/*!< no leading block */
#define NXT_BLK_ERR	(4)	/*!< generic error */

/* Bit mask of verbosity levels set in vflag */
#define VERB_LVL	(1)	/* verbose */
#define VERB_FILE_RDLVL	(2)	/* verbose on record read processing */
#define VERB_FILE_WRLVL	(4)	/* verbose on record write processing */
#define VERB_QUEUE_LVL	(8)	/* squawk about queue handling */
#define VERB_DEBUG_LVL	(16)	/* generic debug statements */
#define VERB_BLOCK_LVL	(32) /* squawk during block processing */
#define VERB_DEBUG_U32	(64) /* squawk about what getu32() does */

#define	LABEL_SIZE	80

/* Default number of lookahead buffers. */
#ifndef MAX_BUFFCOUNT
#define MAX_BUFFCOUNT (10)
#endif

struct vmb_ctx
{
	struct file_details file;
	char *tapefile;
	time_t secs_adj;
	int fd;				/* tape file descriptor */
	int cDelim, dflag, eflag, iflag, Iflag, lcflag, nflag, binaryFlag, tflag, vflag, wflag, xflag, Rflag, vfcflag;
	int setnr, selset, skipSet, numHdrs, saveSet_errors, total_errors;
	char selsetname[14];
	int skipping;
	char label[32768 + LABEL_SIZE];
	int blocksize;
	int buffalloc;
	int num_buffers;
	int buff_cnt;
	struct buff_ctl *buffers;
	int freebuffs;
	int busybuffs;
	int num_busys;
	char **gargv;
	int goptind;
	int gargc;
	int tape_marks;			/* running bit mask of tape marks read */
};

static struct vmb_ctx g_ctx;
static unsigned long last_block_number;

struct buff_ctl
{
	unsigned char *buffer;	/*!< pointer to buffer */
	int next;			/*!< index to next buffer (kept as index so we can realloc if necessary) */
	int amt;			/*!< amount of data in this buffer (0=tape mark) */
	unsigned long blknum;	/*!< block number (stored here for ease of use) */
};

/* Byte-swapping routines.  Note that these do not depend on the size
   of datatypes such as short, long, etc., nor do they require us to
   detect the endianness of the machine we are running on.  It is
   possible they should be macros for speed, but I'm not sure it is
   worth bothering.  We don't have signed versions although we could
   add them if needed.  They are, of course little-endian as that is
   the byteorder used by all integers in a BACKUP saveset.  */

static unsigned long getu32 ( struct vmb_ctx *ctx, unsigned char *addr )
{
	unsigned long ans;
	ans = addr[3];
	ans = (ans<<8) | addr[2];
	ans = (ans<<8) | addr[1];
	ans = (ans<<8) | addr[0];
	if ( (ctx->vflag&VERB_DEBUG_U32) )
		printf("getu32(): %p=%02X %02X %02X %02x = 0x%lX (%ld)\n", (void *)addr, addr[0], addr[1], addr[2], addr[3], ans, ans);
	return ans;
}

static unsigned int getu16 ( struct vmb_ctx *ctx, unsigned char *addr )
{
	unsigned int ans;
	ans = (addr[1] << 8) | addr[0];
	if ( (ctx->vflag&VERB_DEBUG_U32) )
		printf("getu16(): %p=%02X %02X = 0x%X (%d)\n", (void *)addr, addr[0], addr[1], ans, ans);
	return ans;
}

#define GETU16(ctx,x) getu16( ctx, (unsigned char *)&(x) )

/**
 * Dump the contents (indices only) of the busy and free queues for debugging.
 */
static __attribute__((unused)) void dump_queues( struct vmb_ctx *ctx, int which )
{
	if ( (ctx->vflag&VERB_QUEUE_LVL) )
	{
		int idx;
		struct buff_ctl *bptr;
		if ( (which&1) )
		{
			printf( " Busy queue (%d): ", ctx->busybuffs );
			idx = ctx->busybuffs;
			while ( idx )
			{
				printf( "%d ", idx );
				bptr = ctx->buffers + idx;
				idx = bptr->next;
			}
			printf( "\n" );
		}
		if ( (which&2) )
		{
			printf( " Free queue (%d): ", ctx->freebuffs );
			idx = ctx->freebuffs;
			while ( idx )
			{
				printf( "%d ", idx );
				bptr = ctx->buffers + idx;
				idx = bptr->next;
			}
			printf( "\n" );
		}
	}
}

static __attribute__((unused)) struct buff_ctl *popbusy_buff(struct vmb_ctx *ctx)
{
	struct buff_ctl *bptr;
	if ( !ctx->busybuffs )
	{
		if ( (ctx->vflag&VERB_QUEUE_LVL) )
		{
			printf( "popbusy_buff(): No items on queue.\n" );
			dump_queues(ctx, 3 );
		}
		return NULL;
	}
	bptr = ctx->buffers + ctx->busybuffs;
	ctx->busybuffs = bptr->next;
	bptr->next = 0;
	--ctx->num_busys;
	if ( (ctx->vflag&VERB_QUEUE_LVL) )
	{
		printf( "popbusy_buff(): popped %ld off busy queue. num_busys now %d\n",
				(long)(bptr-ctx->buffers), ctx->num_busys );
		dump_queues(ctx, 3 );
	}
	return bptr;
}

static __attribute__((unused)) void add_busybuff( struct vmb_ctx *ctx, struct buff_ctl *bptr, int front )
{
	int prev, ii;

	++ctx->num_busys;
	if ( (ctx->vflag&VERB_QUEUE_LVL) )
	{
		printf( "add_busybuff(): Added item %ld to %s of busy queue. num_busys now %d\n",
				(long)(bptr - ctx->buffers), front ? "head" : "tail", ctx->num_busys );
	}
	if ( front )
	{
		bptr->next = ctx->busybuffs;
		ctx->busybuffs = bptr-ctx->buffers;
	}
	else
	{
		prev = 0;
		ii = ctx->busybuffs;
		bptr->next = 0;		/* make sure this is off */
		while ( ii )		/* walk the chain */
		{
			prev = ii;		/* remember this */
			ii = ctx->buffers[ii].next;	/* advance to next */
		}
		if ( prev )			/* if there was a chain */
			ctx->buffers[prev].next = bptr-ctx->buffers; /* link it */
		else
			ctx->busybuffs = bptr-ctx->buffers; /* else we are on top */
	}
	dump_queues(ctx, 3 );
}

static __attribute__((unused)) struct buff_ctl *getfree_buff(struct vmb_ctx *ctx)
{
	struct buff_ctl *bptr;
	if ( !ctx->freebuffs )
	{
		if ( (ctx->vflag&VERB_QUEUE_LVL) )
		{
			printf( "getfree_buff(): Nothing on free list!!! num_busys now %d\n", ctx->num_busys );
			dump_queues(ctx, 3 );
		}
		return NULL;
	}
	bptr = ctx->buffers + ctx->freebuffs;
	ctx->freebuffs = bptr->next;
	bptr->next = 0;
	bptr->amt = 0;
	bptr->blknum = 0;
	if ( (ctx->vflag&VERB_QUEUE_LVL) )
	{
		printf( "getfree_buff(): Extracted %ld from freelist. num_busys now %d\n",
				(long)(bptr-ctx->buffers), ctx->num_busys );
		dump_queues(ctx, 3 );
	}
	return bptr;
}

static void free_buff( struct vmb_ctx *ctx, struct buff_ctl *bptr )
{
	if ( bptr )
	{
		bptr->next = ctx->freebuffs;
		ctx->freebuffs = bptr - ctx->buffers;
		if ( (ctx->vflag&VERB_QUEUE_LVL) )
		{
			printf( "free_buff(): Put %ld on freelist. num_busys now %d\n", (long)(bptr - ctx->buffers), ctx->num_busys );
			dump_queues(ctx, 3 );
		}
	}
}

static __attribute__((unused)) void freeall( struct vmb_ctx *ctx )
{
	if ( ctx->buffers )
	{
		struct buff_ctl *bp;
		int ii;
		bp = ctx->buffers+1;
		for ( ii=1; ii < ctx->num_buffers-1; ++ii, ++bp )
			bp->next = ii+1;
		bp->next = 0;
		ctx->freebuffs = 1;
		ctx->busybuffs = 0;
		if ( (ctx->vflag&VERB_QUEUE_LVL) )
		{
			printf( "freeall(): Free'd all buffers.\n" );
			dump_queues(ctx, 3 );
		}
	}
}

static __attribute__((unused)) void remove_dups( struct vmb_ctx *ctx )
{
	int ii, lim, jj;
	struct buff_ctl *bptr, *la;
	int buffs[10];		/* place to hold clone of buffer layout (MAX_BUFFCOUNT was 10) */

	if ( ctx->num_busys <= 1 )
		return;				/* nothing to do if there's only one item */
	memset( buffs, 0, sizeof(buffs) );	/* preclear local array */
	buffs[0] = ctx->busybuffs;
	bptr = ctx->buffers + ctx->busybuffs;
	lim = 1;
	while ( lim < 10 && bptr->next )	/* clone the busy list to our local array */
	{
		buffs[lim++] = bptr->next;
		la = ctx->buffers + bptr->next;
		bptr = la;
	}
	if ( lim >= 10 && bptr->next )
	{
		printf( "Snark: fatal internal error. Too many items on buffer list.\n" );
		ctx->skipping |= SKIP_TO_SAVESET;/* toss the remainder of this saveset */
		return;
	}
	if ( lim != ctx->num_busys )
	{
		printf( "Snark: fatal internal error. busy list count (%d) != num_busys (%d).\n",
				lim, ctx->num_busys );
		ctx->skipping |= SKIP_TO_SAVESET;/* toss the remainder of this saveset */
		return;
	}
	if ( lim == 2 && !bptr->amt )	/* only two items, but second is a tm */
		return;/* so, nothing to do */
	for ( ii=0; ii < lim-1; ++ii )	 /* for each item in busy list */
	{
		bptr = ctx->buffers + buffs[ii];
		for ( jj=ii+1; jj < lim; ++jj )	 /* check to see if there's a like numbered one that came later */
		{
			la = ctx->buffers + buffs[jj];
			if ( la->amt && la->blknum == bptr->blknum )
			{
				if ( (ctx->vflag&VERB_FILE_RDLVL) )
					printf( "Found duplicate block numbered %ld. Discarded original.\n", bptr->blknum );
				buffs[ii] = buffs[jj];	/* Just place the duplicate in the original's location */
				if ( lim-jj > 1 )	/* and shift what's left (if any) up one spot */
					memmove( buffs+jj, buffs+jj+1, (lim-jj-1) * sizeof(int) );
				--ii;			/* conteract the outer for loop's ++ */
				--lim;			/* shrink the total by 1 */
				--ctx->num_busys;		/* reduce busy count too */
				if ( (ctx->vflag&VERB_QUEUE_LVL) )
				{
					printf( "Found duplicate block numbered %ld. Discarding buffer %ld\n",
						bptr->blknum, (long)(bptr-ctx->buffers) );
					ctx->busybuffs = buffs[0]; /* fixup the busy que so it'll display correctly */
					for ( jj=0; jj < lim-1; ++jj )
					{
						la = ctx->buffers + buffs[jj];
						la->next = buffs[jj+1];
					}
					la = ctx->buffers + buffs[jj];
					la->next = 0;
				}
				free_buff( ctx, bptr );	/* toss the original buffer */
				break;		/* look again from the beginning */
			}
		}
	}
	if ( (ctx->vflag&VERB_QUEUE_LVL) )
	{
		printf( "After checking for duplicates:\n Busy queue (%d): ", ctx->busybuffs );
		for ( ii=0; ii < lim; ++ii )
			printf( "%d ", buffs[ii] );
		printf( "\n blknums: " );
		for ( ii=0; ii < lim; ++ii )
		{
			bptr = ctx->buffers + buffs[ii];
			printf( "%7ld ", bptr->blknum );
		}
		printf( "\n" );
		dump_queues(ctx, 2 );
	}
	/* Now check the busy list for holes */
	for ( ii=0; ii < lim-1; ++ii )
	{
		bptr = ctx->buffers + buffs[ii];
		la = ctx->buffers + buffs[ii+1];
		if ( la->blknum-bptr->blknum > 1 )
		{
			printf( "Snark: missing block(s) %ld..%ld [%d missing].\n",
					bptr->blknum+1, la->blknum-1, (int)(la->blknum - bptr->blknum - 1) );
			if ( !ctx->eflag && !ctx->xflag )
			{
				ctx->skipping |= SKIP_TO_FILE;
				return;/* abort during listing */
			}
		}
	}
}

/**
 * Get a record from tape or disk.
 *
 * Will not advance beyond two consecutive tape marks.
 *
 * @return 0 for tape mark, >0 bytes read, <0 error.
 */
static int read_record( struct vmb_ctx *ctx, unsigned char *buff, int len )
{
	unsigned char freclen[4], iFreclen[4];
	int sts, tmpLen, reclen, Ireclen;
	int tmpRecCnt;

	if ( (ctx->tape_marks&3) == 3 )
	{
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
			printf( "read_record: returns 0 cuz read 2 TMs in a row.\n" );
		return 0;				/* reached EOT, can't advance */
	}
	ctx->tape_marks <<= 1;
	if ( !ctx->iflag && !ctx->Iflag )
	{
		sts = read( ctx->fd, buff, len );		/* Read from the tape */
		if ( sts <= 0 )				/* A 0 is a tape mark, a -x is an error */
		{
			ctx->tape_marks |= 1;
		}
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
			printf( "read_record: returns %d.\n", sts );
		return sts;
	}
/*
 * Format of our 'i' disk image of a tape is:
 *     4 byte record length in bytes, little endian, followed by 'n' bytes of data.
 * Format of simh 'I' disk image of a tape is the same except it also has the 4 byte count following the 'n' bytes of data. TM's excluded.
 */
	sts = read( ctx->fd, freclen, 4 );		/* Read the record length from disk */
	if ( sts <= 0 )				/* A 0 is EOF. a -x is an error */
	{
		ctx->tape_marks |= 1;			/* pretend we got a tape mark */
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
			printf( "read_record: returns %d due to error or EOF.\n", sts );
		return sts;
	}
	reclen = getu32( ctx, freclen );			/* convert endianess as appropriate */
	if ( !reclen )				/* A 0 length record is a fake tape mark */
	{
		ctx->tape_marks |= 1;			/* Reached a fake tape mark */
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
			printf( "read_record: returns 0 cuz found fake TM.\n" );
		return 0;
	}
	if ( reclen > len )
	{
		printf( "Snark: WARNING: Record of %d bytes too long for user %d buffer.\n", reclen, len );
		sts = read( ctx->fd, buff, len );		/* give 'em what he wants */
		lseek( ctx->fd, reclen-len, SEEK_CUR );	/* Skip remainder of record */
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
			printf( "read_record: returns %d.\n", sts );
		return sts;
	}
	if ( reclen < len )
		len = reclen;				/* trim byte count to actual record length */
	tmpRecCnt = 0;
	tmpLen = 0;
	while ( tmpLen < len )
	{
		sts = read(ctx->fd, buff+tmpLen, len-tmpLen);
		if ( sts <= 0 )
		{
			ctx->tape_marks |= 1;			/* pretend we got a tape mark */
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf( "read_record: read(%d) returns %d due to error or EOF on attempt %d. tmpLen=%d\n", len-tmpLen, sts, tmpRecCnt, tmpLen );
			return sts;
		}
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
			printf( "read_record: attempt %d, read(%d) returns %d.\n", tmpRecCnt, len-tmpLen, sts );
		tmpLen += sts;
		++tmpRecCnt;
	}
	if ( ctx->Iflag )
	{
		sts = read( ctx->fd, iFreclen, 4 );		/* Read the end of block record length from disk */
		if ( sts <= 0 )				/* A 0 is EOF. a -x is an error */
		{
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf( "read_record: returns %d due to error reading SIMH record length.\n", sts );
			return sts;
		}
		Ireclen = getu32( ctx, iFreclen );			/* convert endianess as appropriate */
		if ( Ireclen != reclen )				/* it better match the starting one */
		{
			printf( "Snark: read_record: SIMH format record count mismatch. Expected %d read %d\n",  reclen, Ireclen);
			return -1;
		}
	}
	sts = reclen;
	if ( (ctx->vflag&VERB_DEBUG_U32) || ((ctx->vflag&VERB_BLOCK_LVL ) && !(ctx->vflag&VERB_DEBUG_LVL)) )
		printf("read_record: block returned %d(0x%X)\n", sts, sts );
	return sts;
}

static __attribute__((unused)) void skip_to_tm( struct vmb_ctx *ctx )
{
	while ( 1 )
	{
		if ( !read_record( ctx, (unsigned char *)ctx->label, sizeof( ctx->label ) ) )
			break;
	}
}

static __attribute__((unused)) void alloc_buffers( struct vmb_ctx *ctx, int nbuffs, int buffsize )
{
	int ii;
	struct buff_ctl *bp;

	buffsize += 16;				/* make this a little bigger than he asked for */
	nbuffs = MAX_BUFFCOUNT;			/* always use a fixed number of buffers */
	if ( nbuffs+1 > ctx->num_buffers )		/* need to malloc (more) buffers */
	{
		int jj, newsz;
		ii = ctx->num_buffers;			/* old top */
		ctx->num_buffers = nbuffs+1;			/* new top */
		newsz = ctx->num_buffers*sizeof( struct buff_ctl );	/* figure out how much we will have */
		ctx->buffers = (struct buff_ctl *)realloc( ctx->buffers, newsz );	/* get new memory */
		if ( !ctx->buffers )
		{
			printf( "Snark: Failed to malloc %d bytes for buffer pointers.\n", newsz );
			exit (1);
		}
		bp = ctx->buffers + ii;			/* point to start of new area */
		jj = ii;
		for ( ; ii < ctx->num_buffers-1; ++ii , ++bp )
		{
			bp->buffer = NULL;			/* start with this empty */
			bp->next = ii+1;
			bp->amt = 0;
		}
		bp->buffer = NULL;
		bp->next = ctx->freebuffs;
		bp->amt = 0;
		ctx->freebuffs = jj;
	}
	bp = ctx->buffers+1;				/* now go through and make sure everybody has a buffptr */
	for ( ii=1; ii < ctx->num_buffers; ++ii, ++bp )
	{
		if ( ctx->buffalloc < buffsize || !bp->buffer )
		{
			bp->buffer = (unsigned char *)realloc( bp->buffer, buffsize );
			if ( !bp->buffer )
			{
				printf( "Snark: Failed to malloc %d bytes for block buffer # %d\n", 
						buffsize, ii );
				exit(1);
			}
		}
	}
	if ( ctx->buffalloc < buffsize )
		ctx->buffalloc = buffsize;			/* bump this up if appropriate */
}


static int getRfmRatt(struct file_details *f, char *rcdFormat, int dstLen, char delim)
{
	int ii,rLen;
	static struct
	{
		int type;
		const char *name;
	} RcdFmts[] = {
#if 0
		"UNDEF"    // 0
	   ,"FIXED"    // 1
	   ,"VAR"      // 2
	   ,"VFC"      // 3
	   ,"STMCRLF"  // 4
	   ,"STMLF"	// 5
	   ,"STMCR"	// 6
#endif
		{ FAB_dol_C_RAW, "UNDEF" },
		{ FAB_dol_C_FIX, "FIXED" },
		{ FAB_dol_C_VAR, "VAR" },
		{ FAB_dol_C_VFC, "VFC" },
		{ FAB_dol_C_STM, "STMCRLF" },
		{ FAB_dol_C_STMLF, "STMLF" },
		{ FAB_dol_C_STMCR, "STMCR" },
		{ FAB_dol_C_FIX11, "FIXED" }
	};
	static struct 
	{
		int mask;
		const char *name;
	} RcdAtts[] = {
		{ 1<<FAB_dol_V_FTN, "FTN" },
		{ 1<<FAB_dol_V_CR, "CR" },
		{ 1<<FAB_dol_V_PRN, "PRN" },
		{ 1<<FAB_dol_V_BLK, "BLK" }
	};
	rLen = 0;
	for (ii=0; ii < n_elts(RcdFmts);++ii)
	{
		if ( f->savRecFmt == RcdFmts[ii].type )
		{
			if ( f->savRecFmt == FAB_dol_C_VFC )
				rLen += snprintf(rcdFormat + rLen, dstLen - rLen, "%c%s%d%c%d", delim, RcdFmts[ii].name, f->vfcsize, delim, f->recsize);
			else
				rLen += snprintf(rcdFormat + rLen, dstLen - rLen, "%c%s%c%d", delim, RcdFmts[ii].name, delim, f->recsize);
			break;
		}
	}
	if ( ii >= n_elts(RcdFmts) )
		rLen += snprintf(rcdFormat + rLen, dstLen - rLen, "%cUNDEF%c%d", delim, delim, f->recsize);
	if ( (f->recatt&((1<<n_elts(RcdAtts))-1)) )
	{
		for ( ii = 0; ii < n_elts(RcdAtts); ++ii )
		{
			if ( (f->recatt&RcdAtts[ii].mask) )
				rLen += snprintf(rcdFormat + rLen, dstLen - rLen, "%c%s", delim, RcdAtts[ii].name);
		}
	}
	else
		rLen += snprintf(rcdFormat + rLen, dstLen - rLen, "%cNONE", delim);
	rcdFormat[rLen] = 0;
	return rLen;
}

/**
 * Open a unix file.
 *
 * @param ufn Pointer to place to deposit unix filename.
 * @param fn  Pointer to VMS filename.
 * @param dirfile Flag indicating file is a directory.
 *	@arg 0 File is not a directory.
 *	@arg non-zero File is a directory.
 *
 * @return FILE * of open file or NULL if file not opened.
 *
 * @note
 * Convert VMS filename, 'fn', to unix filename, 'ufn'.
 * Check filename against the 'don't extract' list.
 * Opens an output file only if in extract mode.
 */

static char lastFileName[256];
static int lastVersionNumber;

static FILE *openfile ( struct vmb_ctx *ctx, struct file_details *f )
{
	char ans[80];
	char *p, *q, s, *ext = NULL; /*, *justFileName; */
	int procf;
	char *ufn = f->ufname;
	char *fn = f->name;
	int dirfile = f->directory;
	char rfm[MAX_FORMAT_LEN+4];

	procf = 1;
	/* copy whole fn to ufn and convert to lower case */
	p = fn;
	q = ufn;
	if ( *p == '[' )	/* strip off leading '[' */
		++p;
	while ( *p )
	{
		if ( ctx->lcflag && isupper ( *p ) )
			*q = tolower( *p );
		else
			*q = *p;
		++p;
		++q;
	}
	*q = '\0';

	/* convert the VMS to UNIX and make the directory path */
	p = ufn;
	q = p;
	while ( *q )
	{
		if ( *q == '.' || *q == ']' )
		{
			s = *q;
			*q = '\0';
			if ( procf && ctx->dflag )
				MKDIR( p, 0777 );
			*q = '/';
			if ( s == ']' )
				break;
		}
		++q;
	}
	++q;	/* both ufn and p point to path and q points to start of filename in the ufn string. */
	/* Make a copy of the directory */
/*	justFileName = q; */
	if ( !ctx->dflag )
	{
		strcpy( ufn, q );	/* not keeping the directory structure so toss the path */
		f->altUPfName[0] = '.'; /* alternate file starts with a '.' */
		strncpy(f->altUPfName+1, q, sizeof(f->altUPfName)-2);
	}
	else
	{
		int sLen = q-ufn;	/* Get length of path */
		memcpy(f->altUPfName, ufn, sLen); /* duplicate the path */
		f->altUPfName[sLen] = '.'; /* start alternate filename with a '.' */
		f->altUfNameOnly = f->altUPfName+sLen;
		strncpy(f->altUPfName + sLen + 1, q, sizeof(f->altUPfName)-sLen-2); /* copy rest of filename */
	}
	/* strip off the version number and possibly fix the filename's case */
	while ( *q && *q != ';' )
	{
		if ( *q == '.' )
			ext = q;
		q++;
	}
	f->do_binary = 0;
	f->do_rat = 0;
	if ( !ctx->binaryFlag )
	{
		const char *snarkMsg=NULL;
		f->do_rat = (f->recatt & ((1 << FAB_dol_V_FTN) | (1 << FAB_dol_V_CR) | (1 << FAB_dol_V_PRN)));
		if ( ((f->recfmt & 0x1F) == FAB_dol_C_FIX) || ((f->recfmt & 0x1F) == FAB_dol_C_FIX11) )
			snarkMsg = "Snark: process_file(): File %s is FIXED. Setting it to binary\n";
		if ( !snarkMsg && /* ((file->recfmt & 0x1F) == FAB_dol_C_VAR) && */ !f->do_rat )
			snarkMsg = "Snark: process_file(): File %s has no record attibutes. Setting it to binary\n";
		if ( snarkMsg )
		{
			printf(snarkMsg, f->ufname);
			f->savRecFmt = f->recfmt;
			f->recfmt = FAB_dol_C_RAW;
			f->do_binary = 1;
		}
	}
	else
	{
		f->recfmt = FAB_dol_C_RAW;
		f->do_binary = 1;
	}
	if ( *q == ';' )
	{
		f->versionPtr = q;
		if ( ctx->Rflag )
		{
			char *endp = NULL;
			int curVersion;
			curVersion = strtol(q+1,&endp,10);
			if ( !endp || *endp )
				curVersion = 0;
			*q = 0;
			if ( curVersion && !strcmp(lastFileName,ufn) )
			{
				if ( curVersion < lastVersionNumber )
				{
					printf( "Skipping extraction of \"%s;%d\" because it's an older version of %s;%d.\n", p, curVersion, lastFileName, lastVersionNumber );
					procf = 0;
				}
				else
					lastVersionNumber = curVersion;
			}
			else
			{
				strncpy(lastFileName,ufn,sizeof(lastFileName));
				lastVersionNumber = curVersion;
			}
			*q = ';';
		}
	}
	if ( ctx->Rflag )
		*q = '\0';
	else if ( ctx->cDelim )
		*q = ctx->cDelim;
	getRfmRatt(f,rfm,sizeof(rfm),ctx->cDelim);
	if ( f->do_binary )
	{
		strncat(ufn, rfm, sizeof(f->ufname)-1);
		f->savRecFmt = f->recfmt;
		f->recfmt = FAB_dol_C_RAW;
		f->do_binary = 1;
		f->altUPfName[0] = 0;
		f->altUfNameOnly = NULL;
	}
	else
	{
		strncat(f->altUPfName, rfm, sizeof(f->altUPfName) - 1);
	}
	if ( procf )
	{
		if ( dirfile )
		{
			procf = 0;			/* never explicitly extract directory files */
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
			{
				printf( "Skipping explicit extraction of \"%s\" because it's a directory.\n", p );
			}
		}
		else
		{
			if ( ext && procf )
			{
				procf = typecmp(++ext, ctx->eflag);
			}
		}
	}
	if ( procf && ctx->wflag )
	{
		printf ( "extract %s [ny]", p );
		fflush ( stdout );
		fgets ( ans, sizeof ( ans ), stdin );
		if ( *ans != 'y' )
			procf = 0;
	}
	if ( procf )
	{
		FILE *fp;
		fp = fopen(p,"wb");
		if ( !fp )
		{
			printf("Snark: Failed to open '%s' for output: %s\n", f->ufname, strerror(errno));
		}
		else if ( !ctx->binaryFlag && f->altUPfName[0])
		{
			f->altf = fopen(f->altUPfName,"wb");
			if ( !f->altf )
			{
				fclose(fp);
				unlink(f->ufname);
				printf("Snark: Failed to open '%s' for output: %s\n", f->altUPfName, strerror(errno));
				fp = NULL;
			}
		}
		return fp;
	}
	else
		return( NULL );
}

/**
 * Check filetype against list of 'don't extract'.
 *
 * @param str Pointer to null terminated lowercased filetype.
 *
 * @return 
 *	@arg 0 if file is to be ignored.
 *	@arg 1 if file is not to be ignored.
 *
 * @note
 * Compares the filename type in pointed to by str
 * with our list of file types to be ignored.
 */

int typecmp ( const char *str, int which )
{
	static const char * const Types0[] = {
		"exe",			/* executable image */
		"lib",			/* object library */
		"obj",			/* object file */
		NULL
	};
	static const char * const Types1[] = {
		"odl",			/* rsx overlay description file */
		"olb",			/* rsx object library */
		"pmd",			/* rsx post mortem dump */
		"sys",			/* rsx bootable system image */
		"tlb",			/* ? */
		"tlo",			/* ? */
		"tsk",			/* rsx executable image */
		"upd",			/* ? */
		NULL			/* null string terminates list */
	};
	static const char * const Types2[] = {
		"dir",			/* directory file */
		"mai",			/* mail file */
		NULL
	};
	int ii, jj;
	const char * const *list[4];
	const char * const *type;
	
	list[0] = Types2;
	if ( !which )
	{
		list[1] = Types1;
		list[2] = Types0;
		jj = 3;
	}
	else if ( which == 1 )
	{
		list[1] = Types1;
		jj = 2;
	}
	else 
	{
		jj = 1;
	}
	list[jj] = NULL;
	for (jj=0; (type=list[jj]); ++jj)
	{
		for (ii=0; *type; ++ii, ++type )
		{
			if ( strncasecmp ( str, *type, 3 ) == 0 )
				return( 0 );   /* found a match, file to be ignored */
		}
	}
	(void)ii;
	(void)jj;
	return 1;	   /* no match found keep file */
}

/**
 * Close file.
 *
 * @return Nothing.
 *
 * @note
 * Closes the file opened previously with openfile. Reports any straggling
 * errors if they can be detected at this point.
 */

static void close_file_ctx( struct vmb_ctx *ctx )
{
	ctx->skipping &= ~SKIP_TO_FILE;
	if ( !ctx->file.directory && !(ctx->file.savRecFmt&FAB_dol_M_MAIL) )
	{
		if ( (ctx->xflag || ctx->file.inboundIndex) && ctx->file.inboundIndex != ctx->file.size )
		{
			printf( "Snark: '%s' file size is not correct. Is %d, should be %d. May be corrupt.\n",
					ctx->file.name, ctx->file.inboundIndex, ctx->file.size );
			++ctx->file.file_size_error;
		}
		if ( (ctx->vflag & VERB_FILE_RDLVL) )
		{
			printf( "File size: %d(0x%X), inboundIndex: %d(0x%X), outbountIndex: %d(0x%X), padding: %d, rec_count: %d\n",
					ctx->file.size,
					ctx->file.size,
					ctx->file.inboundIndex,
					ctx->file.inboundIndex,
					ctx->file.outboundIndex,
					ctx->file.outboundIndex,
					ctx->file.rec_padding,
					ctx->file.rec_count );
		}
	}
	if ( ctx->file.extf != NULL )
	{
		struct utimbuf ut;

		fclose ( ctx->file.extf );
		ctx->file.extf = NULL;
		ut.actime = ctx->file.atime;
		ut.modtime = ctx->file.mtime;
		utime( ctx->file.ufname, &ut );
		if ( ctx->file.altf )
		{
			fclose(ctx->file.altf);
			ctx->file.altf = NULL;
			utime(ctx->file.altUPfName, &ut);
		}
		if ( (!ctx->binaryFlag && ctx->file.do_binary) || ctx->file.file_record_error || ctx->file.file_size_error || ctx->file.file_blk_error || ctx->file.file_format_error )
		{
			char refilename[MAX_FILENAME_LEN+MAX_FORMAT_LEN+32];
			int rLen, sLen, rName=0;

			if ( ctx->file.altUPfName[0] )
			{
				int pLen = ctx->file.altUfNameOnly-ctx->file.altUPfName; /* length of path */
				int sLen = strlen(ctx->file.altUfNameOnly+1);
				memcpy(refilename,ctx->file.altUPfName,pLen); /* copy path only */
				refilename[pLen] = 0; /* terminate path */
				memcpy(refilename + pLen, ctx->file.altUfNameOnly+1, sLen); /* copy name sans leading '.' */
				refilename[pLen+sLen] = 0; /* terminate filename */
			}
			else
			{
				strncpy(refilename, ctx->file.ufname, sizeof(refilename) - 1);
			}
			sLen = rLen = strlen(refilename);
			if ( ctx->file.file_record_error )
			{
				if ( ctx->file.altUPfName[0] )
					rLen += snprintf(refilename + rLen, sizeof(refilename) - rLen, "%cisCorruptAt%c%d", ctx->cDelim, ctx->cDelim, ctx->file.altErrorIndex);
				else
					rLen += snprintf(refilename + rLen, sizeof(refilename) - rLen, "%cisCorruptAt%c%d", ctx->cDelim, ctx->cDelim, ctx->file.errorIndex);
			}
			else if ( ctx->file.file_size_error )
				rLen += snprintf(refilename + rLen, sizeof(refilename) - rLen, "%cwrongSize", ctx->cDelim);
			else if ( ctx->file.file_blk_error )
				rLen += snprintf(refilename + rLen, sizeof(refilename) - rLen, "%cfailedBlkDecode", ctx->cDelim);
			else if ( ctx->file.file_format_error )
				rLen += snprintf(refilename + rLen, sizeof(refilename) - rLen, "%cunknownRFM", ctx->cDelim);
			if ( rLen != sLen )
			{
				rName = 1;
				rename( ctx->file.ufname, refilename );
			}
			if ( ctx->file.altUPfName[0] )
			{
				if ( rename( ctx->file.altUPfName, refilename ) == 0 )
					rName |= 2;
			}
			if ( rName == 1 )
				unlink(ctx->file.altUPfName);
			else if ( rName == 2 )
				unlink(ctx->file.ufname);
		}
	}
}

static time_t vms2unixsecs( struct vmb_ctx *ctx, unsigned char *text );
static const char *vms2unixtime( time_t unixtime );

static time_t vms2unixsecs( struct vmb_ctx *ctx, unsigned char *text )
{
	unsigned long long vmstime, vmsepoch;

	vmstime = getu32(ctx, text+4);
	vmstime = (vmstime<<32) | getu32(ctx, text);
	if ( !vmstime )
		return(time_t)0;   /* no time specified */
	vmstime /= 10000000LL;	/* Compute vms time in seconds (10^7 ticks per second) */
	/* Get seconds between VMS epoch, 17-nov-1858:00:00:00 and unix epoch 1-jan-1970:00:00:00 */
	vmsepoch = 3506716800LL;
#if 0
	vmsepoch += 139651200LL;
#endif
	vmstime -= vmsepoch;	/* convert to Unix epoch (1-jan-1970 00:00:00 GMT) */
	return(time_t)vmstime;
}

static const char *vms2unixtime( time_t vtime )
{
	char *ans, *cp;
	if ( !vtime )
		return "<none specified>";
	cp = ans = ctime( &vtime );
	while ( *cp )
	{
		if ( *cp == '\n' )	/* clip off trailing newline */
		{
			*cp = 0;
			break;
		}
		++cp;
	}
	return ans;
}

static void process_file_ctx(struct vmb_ctx *ctx, unsigned char *buffer, int rsize)
{
	unsigned char *data;
	short dsize, dtype;
	int ii, cc, subf=0;
	int procf = 0;

	close_file_ctx(ctx);

	/* check the header word */
	if ( buffer[0] != 1 || buffer[1] != 1 )
	{
		printf ( "Snark: invalid file record header. Expected 01 01, found %02X %02X\n", buffer[0], buffer[1] );
		ctx->skipping |= SKIP_TO_FILE; /* Skip to next file block */
		++ctx->saveSet_errors;
		return;
	}

	cc = 2;
	while ( cc <= rsize-4 )
	{
		struct bsa *bsa;

		bsa = (struct bsa *)( buffer + cc );
		dsize = GETU16( ctx, bsa->bsa_dol_w_size );
		dtype = GETU16( ctx, bsa->bsa_dol_w_type );
		data  = (unsigned char *)bsa->bsa_dol_t_text;

		if ( dsize < 0 || dsize+cc+4 > rsize )
		{
			printf( "Snark: process_file() subfield %d, type %d, found bad count of %d.\n", subf, dtype, dsize );
			++ctx->saveSet_errors;
			ctx->skipping |= SKIP_TO_FILE; /* skip to next file block */
			return;
		}
		switch ( (int)dtype )
		{
		case FREC_END:
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
				printf( "File record field %2d, type END, size %d.\n", subf, dsize );
			cc = rsize+1;
			break;
		case FREC_FNAME:
			if ( dsize >= (int)sizeof(ctx->file.name) )
				dsize = sizeof(ctx->file.name)-1;
			memcpy ( ctx->file.name, data, dsize );
			ctx->file.name[dsize] = '\0';
			cc = rsize+1; /* force break out of loop */
			break;
		case FREC_UID:
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
			{
				int uid, gid;
				uid = ((data[3]&0xFF)<<8) | (data[2]&0xFF);
				gid = ((data[5]&0xFF)<<8) | (data[4]&0xFF);
				printf( "File record field %2d, type UID, size %d. \"%s\" %06o,%06o\n",
						subf, dsize, vms2unixtime(ctx->file.atime), gid, uid );
			}
			break;
		case FREC_FORMAT:
			if ( dsize < 16 )
			{
				printf( "Snark: process_file(): subfield %d, type FORMAT, size %d. Bad format count.\n",
						subf, dsize );
				++ctx->saveSet_errors;
				ctx->skipping |= SKIP_TO_FILE; /* skip to next file block */
				return;
			}
			ctx->file.recfmt = data[0];
			ctx->file.recatt = data[1];
			ctx->file.recsize = getu16( ctx, data+2 );
			ctx->file.nblk = getu16( ctx, data+10 )
			    + (64 * 1024) * getu16( ctx, data+8 );
			ctx->file.lnch = getu16( ctx, data+12 );
			if ( !ctx->file.nblk )
				ctx->file.size = 0;
			else
				ctx->file.size = (ctx->file.nblk-1) * 512 + ctx->file.lnch;
			ctx->file.vfcsize = data[15];
			if ( ctx->file.vfcsize == 0 )
				ctx->file.vfcsize = 2;
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
			{
				printf( "File record field %2d, type FORMAT, size %d. fmt %d, att %d, rsiz %d\n",
						subf, dsize, ctx->file.recfmt, ctx->file.recatt, ctx->file.recsize );
				printf( "                  nblk %d, lnch %d, vfcsize %d, filesize %d\n",
					 ctx->file.nblk, ctx->file.lnch, ctx->file.vfcsize, ctx->file.size );
			}
			break;
		case FREC_CTIME:
			if ( dsize >= 8 )
				ctx->file.ctime = vms2unixsecs( ctx, data );
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
				printf( "File record field %2d, type CTIME, size %d. \"%s\"\n",
						subf, dsize, vms2unixtime( ctx->file.ctime ) );
			break;
		case FREC_MTIME:
			if ( dsize >= 8 )
				ctx->file.mtime = vms2unixsecs( ctx, data );
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
				printf( "File record field %2d, type MTIME, size %d. \"%s\"\n",
						subf, dsize, vms2unixtime( ctx->file.mtime ) );
			break;
		case FREC_ATIME:
			if ( dsize >= 8 )
				ctx->file.atime = vms2unixsecs( ctx, data );
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
				printf( "File record field %2d, type ATIME, size %d. \"%s\"\n",
						subf, dsize, vms2unixtime( ctx->file.atime ) );
			break;
		case FREC_BTIME:
			if ( dsize >= 8 )
				ctx->file.btime = vms2unixsecs( ctx, data );
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
				printf( "File record field %2d, type BTIME, size %d. \"%s\"\n",
						subf, dsize, vms2unixtime( ctx->file.btime ) );
			break;
		case FREC_DIRECTORY:
			ctx->file.directory = data[0];
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
				printf( "File record field %2d, type DIRECTORY, size %d. 0x%02X\n",
						subf, dsize, data[0] );
			break;
		case FREC_UNK2b:
		case FREC_UNK2c:
		case FREC_UNK2d:
		case FREC_UNK2e:
		case FREC_UNK30:
		case FREC_UNK31:
		case FREC_UNK32:
		case FREC_UNK33:
		case FREC_UNK35:
		case FREC_UNK47:
		case FREC_UNK48:
		case FREC_UNK4a:
		case FREC_UNK4b:
		case FREC_UNK4e:
		case FREC_UNK4f:
		case FREC_UNK50:
		case FREC_UNK57:
			if ( (ctx->vflag & VERB_FILE_RDLVL) )
			{
				int jj;
				printf( "File record field %2d (UNK) type 0x%02X, size %d: ",
						subf, dtype, dsize );
				ii = 8;
				if ( ii > dsize )
					ii = dsize;
				for ( jj=0; jj < ii; ++jj )
				{
					printf( "%02X ", data[jj] );
				}
				if ( dsize > 8 )
					printf( " (+%d bytes)\n", dsize-8 );
				else
					printf( "\n" );
			}
			break;
		default:
			printf( "Snark: process_file(): subfield %d, undefined record type: %d size %d\n",
					subf, dtype, dsize );
			++ctx->saveSet_errors;
			break;
		}
		++subf;
		cc += dsize + 4;
	}

	if ( strstr(ctx->file.name,".MAI") )
	{
		ctx->file.recfmt |= FAB_dol_M_MAIL;
		ctx->file.savRecFmt = ctx->file.recfmt;
	}
	procf = 0;
	if ( ctx->goptind < ctx->gargc )
	{
		for ( ii = ctx->goptind; ii < ctx->gargc; ii++ )
		{
			procf |= match ( ctx->file.name, ctx->gargv[ii] );
		}
	}
	else
		procf = 1;
	if ( procf )
	{
		if ( ctx->tflag )
		{
			char rfm[MAX_FORMAT_LEN];
			getRfmRatt(&ctx->file,rfm,sizeof(rfm), ctx->cDelim);
			printf ( " %-35s %8d (%s)%s\n", ctx->file.name, ctx->file.size, rfm, ctx->file.size < 0 ? " (IGNORED!!!)" : "" );
		}
		if ( ctx->file.size < 0 )
		{
			if ( !ctx->tflag && ctx->xflag )
				printf ( "Snark: process_file(): %-35s not extracted due to filesize of %8d\n",
					 ctx->file.name, ctx->file.size );
			++ctx->file.file_size_error;
			++ctx->saveSet_errors;
			ctx->skipping |= SKIP_TO_FILE; /* this is bad */
			return;
		}
		if ( ctx->file.directory || (ctx->file.recfmt&FAB_dol_M_MAIL) )
		{
			if ( (ctx->vflag&VERB_FILE_RDLVL) )
				printf( "Skipping file due to it being a dir or mail file or recsize is 0.\n" );
			ctx->skipping |= SKIP_TO_FILE; /* ignore this file since the types are bogus */
			return;
		}
		if ( ctx->xflag )
		{
			ctx->file.extf = openfile ( ctx, &ctx->file );
			if ( ctx->file.extf != NULL && ctx->vflag )
				printf ( "extracting %s\n", ctx->file.name );
		}
	}
}

void process_file ( unsigned char *buffer, int rsize )
{
	process_file_ctx(&g_ctx, buffer, rsize);
}

static void vmb_ctx_reset(struct vmb_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->cDelim = ';';
}

static void usage ( const char *progname, int details )
{
	printf( "Usage: %s -f <tape image> [-t|-x] [options]\n", progname );
	if ( !details )
		return;
	printf( "Options:\n" );
	printf( "  -t, --list          List contents\n" );
	printf( "  -x, --extract       Extract files (not implemented in this refactor)\n" );
	printf( "  -f, --file <path>   Tape image path\n" );
	printf( "  -h, --help          This help\n" );
}

static int rdhead ( struct vmb_ctx *ctx )
{
	int marks=0, mstop, len, nfound, rptd=0, stm=0;
	char name[80];

	ctx->skipping = 0;
	ctx->total_errors += ctx->saveSet_errors;
	ctx->saveSet_errors = 0;
	nfound = 1;
	mstop = 3;				/* autostop when we get to 2 tm's */
	last_block_number = 0;		/* start all blocks at 0 */
	freeall(ctx);				/* free all the buffers */

	while ( 1 )
	{
		marks <<= 1;
		len = read_record( ctx, (unsigned char *)ctx->label, sizeof(ctx->label) );
		if ( !len )
		{
			++marks;
		}
		if ( marks >= mstop )
		{
			if ( !len )
				return -1;		/* found EOF */
			return 1;			/* no HDR1-2 found, skip it */
		}
		if ( len < LABEL_SIZE )
		{
			if ( len < 0 )
				printf( "Snark: rdhead(): read_record() returned %d. Skipping.\n", len );
			else
				printf( "Snark: rdhead(): bad label count of %d. Skipping.\n", len );
			stm = 2;			/* skip to next saveset */
			continue;
		}
		if ( ctx->label[0] != 'H' )
		{
			printf( "Snark: rdhead(): bad label type = '%c'\n", ctx->label[0] );
			continue;
		}
		if ( !strncmp ( ctx->label, "HDR2", 4 ) )
		{
			++rptd;				/* count up how many times we've hit a HDR2 */
			if ( rptd > 10 )
			{
				printf( "Snark: Too many HDR2 records. (%d)\n", rptd );
				return -1;
			}
			if ( !ctx->skipSet || ctx->selset )
			{
				ctx->skipSet = 0;		/* turn this off */
				ctx->selset = 0;		/* turn this off too */
				if ( (ctx->vflag&VERB_LVL) || ctx->tflag )
				{
					memcpy(name, ctx->label+4, 14);
					name[14] = 0;
					printf ( "Saveset number %d: %s\n\n", ctx->setnr+1, name );
				}
				break;
			}
			stm = 1;			/* skip to next HDR1 */
		}
		else if ( !strncmp ( ctx->label, "HDR1", 4 ) )
		{
			++ctx->setnr;
			marks = 0;			/* reset the mark counter */
			if ( ctx->selset )
			{
				if ( !strncmp( ctx->selsetname, ctx->label+4, 14 ) )
					ctx->selset = 0;
				else
				{
					if ( (ctx->vflag&VERB_LVL) || ctx->tflag )
						printf( "Saveset %s does not match selected saveset %s. Skipping.\n",
								ctx->label+4, ctx->selsetname );
					continue;
				}
			}
			if ( stm == 1 )
			{
				if ( ctx->skipSet && ctx->setnr < ctx->skipSet )
				{
					if ( (ctx->vflag&VERB_LVL) || ctx->tflag )
						printf( "Skipping saveset %d because it's before -s flag of %d.\n", ctx->setnr, ctx->skipSet );
					continue;
				}
				stm = 0;
			}
			if ( stm == 2 )
				continue;
			++ctx->numHdrs;
			if ( ctx->skipSet )
			{
				if ( ctx->numHdrs < ctx->skipSet )
				{
					if ( (ctx->vflag&VERB_LVL) || ctx->tflag )
						printf( "Number of HDRs of %d is less than -S flag of %d. Skipping.\n", ctx->numHdrs, ctx->skipSet );
					stm = 2;			/* skip to next volume or HDR marker */
					continue;
				}
				if ( ctx->numHdrs > ctx->skipSet )
				{
					if ( (ctx->vflag&VERB_LVL) || ctx->tflag )
						printf( "Number of HDRs %d is more than -S flag of %d. Done.\n", ctx->numHdrs, ctx->skipSet );
					nfound = -1;
					break;
				}
			}
			nfound = 0;
			mstop = 1;
			continue;
		}
	}
	if ( rptd > 1 )
		printf( "Snark: rdhead(): Skipped %d bad records looking for a HDR2.\n", rptd );
	if ( !ctx->tflag && (ctx->vflag & VERB_LVL) && !nfound )
		printf ( "Saveset name: %s   number: %d\n", name, ctx->setnr );
	if ( !nfound && ctx->blocksize && ctx->blocksize+16 > ctx->buffalloc )
	{
		alloc_buffers( ctx, MAX_BUFFCOUNT, ctx->blocksize );
		freeall(ctx);
	}
	return( nfound );
}

static int read_next_block( struct vmb_ctx *ctx )
{
	unsigned long numb0;
	struct buff_ctl *bptr;
	int ra, hittm=0;

	if ( !ctx->busybuffs )		/* if first time through, need to rdhead() then fill n buffers */
	{
		if ( rdhead ( ctx ) )	/* read header */
			return NXT_BLK_EOT;	/* reached eot */
		bptr = getfree_buff(ctx);
		if ( !bptr )
		{
			printf( "Snark: Fatal internal error. No more free buffs.\n" );
			ctx->skipping |= SKIP_TO_SAVESET;
			return NXT_BLK_ERR;
		}
		while ( 1 )
		{
			bptr->amt = read_record( ctx, bptr->buffer, ctx->buffalloc );	/* fill first buffer */
			if ( !bptr->amt )
			{
				free_buff(ctx, bptr );				/* put this back */
				return NXT_BLK_TM;				/* found tm */
			}
			if ( bptr->amt == ctx->blocksize )           /* block is ok so far */
			{
				numb0 = get_block_number( ctx, bptr->buffer );	/* get the block number of leading block */
				if ( !numb0 )
					continue;					/* not a valid block, skip it */
				if ( numb0 != 1 )				/* it had better be a 1 */
				{
					free_buff(ctx, bptr );				/* put this back */
					return NXT_BLK_NOLEAD;			/* no leading block */
				}
				break;
			}
			printf ( "Snark: record size incorrect. read amt = %d, expected %d\n", bptr->amt, ctx->blocksize );
		}
		bptr->blknum = 1;					/* always starts with block 1 */
		add_busybuff( ctx, bptr, 0 );				/* put this on the busy queue */
	}
	bptr = ctx->buffers+ctx->busybuffs;		/* point to top item on queue */
	if ( !bptr->amt )			/* if top buffer is a TM */
	{
		free_buff(ctx, popbusy_buff(ctx) );	/* toss the top item */
		return NXT_BLK_TM;		/* return eof */
	}
	while ( bptr->next )		/* find last item on busy queue */
		bptr = ctx->buffers + bptr->next;
	if ( !bptr->amt )			/* if last item is a tm, nothing left to read */
		return NXT_BLK_OK;		/* just consume whatever is currently on the queue */
	while ( !hittm && ctx->num_busys < MAX_BUFFCOUNT )	/* keep busy queue as full as possible */
	{
		for ( ra=ctx->num_busys; !hittm && ra < MAX_BUFFCOUNT; ++ra )	/* may need to readahead n buffers */
		{
			bptr = getfree_buff(ctx);		/* get a free buffer */
			if ( !bptr )
			{
				printf( "Snark: Fatal internal error. Ran out of free buffs.\n" );
				ctx->skipping |= SKIP_TO_SAVESET;
				return NXT_BLK_ERR;
			}
			while ( !hittm )
			{
				bptr->amt = read_record( ctx, bptr->buffer, ctx->buffalloc );	/* fill it up */
				if ( !bptr->amt )		/* reached TM on readahead */
				{
					hittm = 1;			/* can't read anymore */
					break;
				}
				if ( bptr->amt == ctx->blocksize )
				{
					bptr->blknum = get_block_number( ctx, bptr->buffer );	/* get the block number */
					if ( !bptr->blknum )	/* not a valid block */
						continue;		/* get another one */
					break;			/* block is ok so far */
				}
				printf ( "Snark: record size on readahead is incorrect. read amt = %d, expected %d\n",
						 bptr->amt, ctx->blocksize );
			}
			if ( !hittm )
				add_busybuff( ctx, bptr, 0 );	/* append the buffer to busy queue */
			else
				free_buff(ctx, bptr );		/* toss this for now */
		}
		remove_dups(ctx);				/* account for missing & duplicates in busy queue */
	}
	if ( hittm )			/* if we've hit a TM */
	{
		bptr = getfree_buff(ctx);
		add_busybuff( ctx, bptr, 0 );	/* stick a TM at end of busy queue */
	}
	return NXT_BLK_OK;			/* we've got a good record */
}

static unsigned long get_block_number( struct vmb_ctx *ctx, unsigned char *bptr )
{
	unsigned long ans = 0, bsize;
	struct bbh *block_header;
	unsigned short bhsize;

	block_header = ( struct bbh * )bptr;

	bhsize = GETU16( ctx, block_header->bbh_dol_w_size );
	bsize = getu32(ctx, (unsigned char *)&block_header->bbh_dol_l_blocksize );

	if ( bhsize != sizeof ( struct bbh ) )
	{
		printf ( "Snark: Invalid header block size. Expected %zu, found %lu\n",
				sizeof( struct bbh ), (unsigned long)bhsize );
		return ans;
	}
	if ( bsize != 0 && bsize != (unsigned long)ctx->blocksize )
	{
		printf ( "Snark: Invalid block size. Expected %d, found %ld\n", ctx->blocksize, bsize );
		return ans;
	}
	return getu32(ctx, (unsigned char *)&block_header->bbh_dol_l_number );
}

static void process_block ( struct vmb_ctx *ctx, unsigned char *blkptr )
{
	void process_vbn ( unsigned char *buffer, int rsize );
	void process_summary( unsigned char *blkptr, int rsize );

	unsigned short rsize, rtype, applic;
	unsigned long bsize, ii, numb;
	struct bbh *block_header;

	ctx->skipping &= ~SKIP_TO_BLOCK;

	/* read the backup block header */
	block_header = ( struct bbh * )blkptr;
	ii = sizeof( struct bbh );

	bsize = getu32(ctx, (unsigned char *)&block_header->bbh_dol_l_blocksize );

	numb = get_block_number( ctx, blkptr );
	if ( !numb )
	{
		ctx->skipping |= SKIP_TO_BLOCK;
		++ctx->saveSet_errors;
		++ctx->file.file_blk_error;
		return;
	}
	if ( numb != last_block_number+1 )
	{
		if ( numb == last_block_number )
			printf( "Snark: block %ld duplicated.\n", numb );
		else
			printf( "Snark: block %ld out of sequence. Expected %ld\n",
					numb, last_block_number+1 );
	}
	last_block_number = numb;
	applic = GETU16( ctx, block_header->bbh_dol_w_applic );
	if ( (ctx->vflag & VERB_DEBUG_LVL) )
	{
		printf ( "new block: ii = %ld, bsize = %ld, opsys=%d, subsys=%d, applic=%d, number=%ld\n",
				 ii, bsize,
				 GETU16( ctx, block_header->bbh_dol_w_opsys ),
				 GETU16( ctx, block_header->bbh_dol_w_subsys ),
				 applic,
				 numb );
	}
	if ( !bsize || applic > 1 )
	{
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
		{
			if ( !bsize )
				printf( "Process_block(): Skipped block because bsize == 0\n" );
			else
				printf( "Process_block(): Skipped block because applic field is %d instead of 1.\n",
						applic );
		}
		ctx->skipping |= SKIP_TO_BLOCK;
		return;
	}
	/* read the records */
	while ( ii < bsize )
	{
		struct brh *record_header;
		/* read the backup record header */
		record_header = ( struct brh * ) (blkptr+ii);
		ii += sizeof ( struct brh );

		rtype = GETU16( ctx, record_header->brh_dol_w_rtype );
		rsize = GETU16( ctx, record_header->brh_dol_w_rsize );
		if ( (ctx->vflag & VERB_DEBUG_LVL) )
		{
			printf ( "ii=%ld, rtype=%d, rsize=%d, flags=0x%lX, addr=0x%lX\n",
					 ii, rtype, rsize,
					 getu32(ctx, (unsigned char *)&record_header->brh_dol_l_flags ),
					 getu32(ctx, (unsigned char *)&record_header->brh_dol_l_address ) );
		}
		if ( rsize+ii > bsize )	/* This is an invalid record */
		{
			printf( "Snark: rsize of %d is wrong. Cannot be more than %ld\n",
					rsize, bsize-ii );
			ctx->skipping |= SKIP_TO_BLOCK;
			++ctx->saveSet_errors;
			++ctx->file.file_record_error;
			break;
		}
		switch ( rtype )
		{
		
		case brh_dol_k_null:
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf ( "rtype = null\n" );
			break;

		case brh_dol_k_summary:
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf ( "rtype = summary\n" );
			process_summary( blkptr+ii, rsize );
			break;

		case brh_dol_k_file:
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf ( "rtype = file\n" );
			process_file ( blkptr+ii, rsize );
			break;

		case brh_dol_k_vbn:
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf ( "rtype = vbn\n" );
			if ( !(ctx->skipping&SKIP_TO_FILE) )
				process_vbn ( blkptr+ii, rsize );
			break;

		case brh_dol_k_physvol:
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf ( "rtype = physvol\n" );
			break;

		case brh_dol_k_lbn:
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf ( "rtype = lbn\n" );
			break;

		case brh_dol_k_fid:
			if ( (ctx->vflag & VERB_DEBUG_LVL) )
				printf ( "rtype = fid\n" );
			break;

		default:
			printf ( "Snark: process_block(): %d is an invalid record type.\n", rtype );
			++ctx->saveSet_errors;
			if ( ctx->file.extf )
			{
				printf( "Snark: Skipping rest of %s\n", ctx->file.name );
				++ctx->file.file_record_error;
			}
			ctx->skipping |= SKIP_TO_BLOCK|SKIP_TO_FILE;
			return;
		}
		ii += rsize;
	}
}

static int vmsbackup_entry(struct vmb_ctx *ctx, int argc, char *argv[])
{
	const char *progname = argv[0];
	int c;
	int option_index = 0;
	char *endp;
	struct stat fileStat;

	if ( argc < 2 )
	{
		usage( progname, 1 );
		return 1;
	}

	vmb_ctx_reset(ctx);

	while ( ( c = getopt_long ( argc, argv, "cdeEF:f:hiIln:Rs:tv:wx", long_options, &option_index ) ) != -1 )
	{
		switch ( c )
		{
		case OPT_VER_DELIMIT:	/* --delimiter */
			if ( !optarg )
			{
				printf("Defaulting version delimiter to ':'\n");
				ctx->cDelim = ':';
				break;
			}
			if ( !isprint((unsigned char)optarg[0]) )
			{
				printf("Argument to --delimiter must be printable. Is 0x%02X\n", (unsigned char)optarg[0]);
				return 1;
			}
			ctx->cDelim = optarg[0];
			break;
		case OPT_EXTRACT:		/* --extract */
			endp = NULL;
			ctx->eflag = strtoul(optarg,&endp,0);
			if ( !endp || *endp || ctx->eflag < 0 || ctx->eflag > 2)
			{
				printf("Snark: Bad --extract parameter: '%s'. Must be a number 0 <= n <= 2\n", optarg);
				return 1;
			}
			++ctx->xflag;
			break;
		case 'c':
			ctx->cDelim = ':';
			break;
		case OPT_HIERARCHY:		/* -d */
		case 'd':
			++ctx->dflag;
			break;
		case 'e':
			ctx->eflag = 1;
			break;
		case 'E':
			ctx->eflag = 2;
			break;
		case OPT_FILE:			/* -f */
		case 'f':
			ctx->tapefile = optarg;
			break;
		case 'F':
			endp = NULL;
			ctx->vfcflag = strtoul(optarg,&endp,0);
			if ( !endp || *endp || ctx->vfcflag < 0 || ctx->vfcflag > 2)
			{
				printf("Snark: Bad -F parameter: '%s'. Must be a number 0 <= F <= 2\n", optarg);
				return 1;
			}
			break;
		case OPT_HELP:			/* -h */
		default:
			printf( "Unrecognised option: '%c'.\n", c );
		case 'h':
		case '?':
			usage ( progname, 1 );
			return 0;
		case OPT_DVD:			/* -i */
		case 'i':
			++ctx->iflag;
			break;
		case OPT_SIMH:			/* -I */
		case 'I':
			++ctx->Iflag;
			break;
		case OPT_BINARY:
			++ctx->binaryFlag;
			break;
		case OPT_LOWERCASE:		/* -l */
		case 'l':
			++ctx->lcflag;
			break;
		case OPT_SET_NAME:		/* -n */
		case 'n':
			++ctx->nflag;
			break;
		case OPT_NO_VERSION:		/* -R */
		case 'R':
			++ctx->Rflag;
			break;
		case OPT_HDR1_NUMBER:	/* -s */
		case 's':
			ctx->skipSet = strtol(optarg,&endp,0);
			if ( !endp || *endp || ctx->skipSet <= 0 )
			{
				printf("Snark: Bad -s parameter. '%s' Must be integer greater than 1\n", optarg);
				return 1;
			}
			break;
		case OPT_LIST:			/* -t */
		case 't':
			++ctx->tflag;
			break;
		case OPT_VERBOSE:		/* -v */
		case 'v':
			endp = NULL;
			ctx->vflag = strtoul(optarg,&endp,0);
			if ( !endp || *endp )
			{
				printf("Snark: Bad -v parameter: '%s'. Must be a number\n", optarg);
				return 1;
			}
			break;
		case OPT_PROMPT:			/* -w */
		case 'w':
			++ctx->wflag;
			break;
		case 'x':
			++ctx->xflag;
			break;
		}
	}

	if ( !ctx->tapefile )
	{
		printf("The -f (or --file) option is required.\n");
		return 1;
	}
	if ( !ctx->tflag && !ctx->xflag )
	{
		printf( "You must provide either -x or -t.\n" );
		usage ( progname, 1 );
		return 1;
	}

	if ( !ctx->tapefile )
	{
		printf("The -f (or --file) option is required.\n");
		return 1;
	}
	if ( !ctx->tflag && !ctx->xflag )
	{
		printf( "You must provide either -x or -t.\n" );
		usage ( progname, 1 );
		return 1;
	}

	ctx->gargv = argv;
	ctx->gargc = argc;
	ctx->goptind = optind;

	ctx->fd = stat( ctx->tapefile, &fileStat);
	if ( ctx->fd < 0 )
	{
		perror("Failed to stat file");
		return 1;
	}
	ctx->fd = open(ctx->tapefile, OPEN_FLAGS);
	if ( ctx->fd < 0 )
	{
		perror ( ctx->tapefile );
		return 1;
	}

	printf("vmsbackup_entry(): core pipeline not yet reimplemented; options parsed and file opened.\n");
	printf("vmsbackup_entry(): core pipeline not yet reimplemented; options parsed.\n");
	return 1;
}

int vmsbackup_main ( int argc, char *argv[] )
{
	return vmb_ctx_run(&g_ctx, argc, argv);
}

vmb_ctx *vmb_ctx_create(void)
{
	vmb_ctx *ctx = (vmb_ctx *)malloc(sizeof(vmb_ctx));
	if (ctx)
	{
		vmb_ctx_reset(ctx);
	}
	return ctx;
}

void vmb_ctx_destroy(vmb_ctx *ctx)
{
	if (ctx)
	{
		free(ctx);
	}
}

int vmb_ctx_run(vmb_ctx *ctx, int argc, char *argv[])
{
	if (!ctx)
		return -1;
	return vmsbackup_entry(ctx, argc, argv);
}

#ifndef VMSBACKUP_NO_MAIN
int main ( int argc, char *argv[] )
{
	return vmsbackup_main(argc, argv);
}
#endif
