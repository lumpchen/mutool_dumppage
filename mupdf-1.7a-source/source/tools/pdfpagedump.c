/*
 * PDF cleaning tool: general purpose pdf syntax washer.
 *
 * Rewrite PDF with pretty printed objects.
 * Garbage collect unreachable objects.
 * Inflate compressed streams.
 * Create subset documents.
 *
 * TODO: linearize document for fast web view
 */

#include "mupdf/pdf.h"

static void usage(void)
{
	fprintf(stderr, "usage: mutool dump test.pdf pagenumber zoom\n");
	
	fprintf(stderr, "usage: mutool dumppage [options] file.pdf dump.xml preview.png [page number]\n");
	fprintf(stderr, "\t-p\tpassword\n");
	fprintf(stderr, "\t-z\tzoom ratio(percentage)\n");
	exit(1);
}

void dump(char *filename, char *dumpfile, char *preview, int pagenumber, char *password, int zoom, int rotation)
{
	fz_context *ctx;
	fz_document *doc;
	int pagecount;
	fz_page *page;
	fz_matrix transform;
	fz_rect bounds;
	fz_irect bbox;
	fz_pixmap *pix;
	fz_device *dev;
	fz_dumped_page *dumped_page;

	// Create a context to hold the exception stack and various caches.
	ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if (!ctx)
	{
		fprintf(stderr, "cannot initialise context\n");
		exit(1);
	}

	// Register document handlers for the default file types we support.
	fz_register_document_handlers(ctx);

	// Open the PDF, XPS or CBZ document.
	doc = fz_open_document(ctx, filename);
	if (fz_needs_password(ctx, doc))
	{
		if (!fz_authenticate_password(ctx, doc, password))
		{
			fz_throw(ctx, FZ_ERROR_GENERIC, "cannot authenticate password: %s", filename);
		}
	}

	// Retrieve the number of pages (not used in this example).
	pagecount = fz_count_pages(ctx, doc);
	if (pagenumber > pagecount)
	{
		fprintf(stderr, "\npage number %d is out of page count %d.\n", pagenumber, pagecount);
		exit(1);
	}

	// Load the page we want. Page numbering starts from zero.
	page = fz_load_page(ctx, doc, pagenumber - 1);

	// Calculate a transform to use when rendering. This transform
	// contains the scale and rotation. Convert zoom percentage to a
	// scaling factor. Without scaling the resolution is 72 dpi.
	fz_rotate(&transform, rotation);
	fz_pre_scale(&transform, zoom / 100.0f, zoom / 100.0f);

	// Take the page bounds and transform them by the same matrix that
	// we will use to render the page.
	fz_bound_page(ctx, page, &bounds);
	fz_transform_rect(&bounds, &transform);

	// Create a blank pixmap to hold the result of rendering. The
	// pixmap bounds used here are the same as the transformed page
	// bounds, so it will contain the entire page. The page coordinate
	// space has the origin at the top left corner and the x axis
	// extends to the right and the y axis extends down.
	fz_round_rect(&bbox, &bounds);
	pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), &bbox);
	fz_clear_pixmap_with_value(ctx, pix, 0xff);

	// A page consists of a series of objects (text, line art, images,
	// gradients). These objects are passed to a device when the
	// interpreter runs the page. There are several devices, used for
	// different purposes:
	//
	//	draw device -- renders objects to a target pixmap.
	//
	//	text device -- extracts the text in reading order with styling
	//	information. This text can be used to provide text search.
	//
	//	list device -- records the graphic objects in a list that can
	//	be played back through another device. This is useful if you
	//	need to run the same page through multiple devices, without
	//	the overhead of parsing the page each time.

	// Create a draw device with the pixmap as its target.
	// Run the page with the transform.

	dumped_page = fz_new_dumped_page(ctx, pagenumber, dumpfile);
	dev = fz_new_cj_device(ctx, pix, dumped_page);
	
	fz_run_page(ctx, page, dev, &transform, NULL);
	fz_drop_device(ctx, dev);

	// Save the pixmap to a file.
	fz_write_png(ctx, pix, preview, 0);

	// Clean up.
	fz_drop_pixmap(ctx, pix);
	fz_drop_page(ctx, page);
	fz_drop_dumped_page(ctx, dumped_page);
	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
}

int pdfdumppage_main(int argc, char **argv)
{
	char *infile, *dumpfile, *preview;
	char *password = "";
	int c;
	int zoom = 100;
	int pagenumber = 1;

	while ((c = fz_getopt(argc, argv, "z:p")) != -1)
	{
		switch (c)
		{
		case 'p': password = fz_optarg; break;
		case 'z': zoom = atoi(fz_optarg); break;
		default: usage(); break;
		}
	}

	if (fz_optind == argc)
	{
		usage();
	}
	infile = argv[fz_optind++];
	
	if (fz_optind == argc)
	{
		usage();
	}
	dumpfile = argv[fz_optind++];

	if (fz_optind == argc)
	{
		usage();
	}
	preview = argv[fz_optind++];

	if (fz_optind == argc)
	{
		usage();
	}
	pagenumber = atoi(argv[fz_optind]);

	dump(infile, dumpfile, preview, pagenumber, password, zoom, 0);
	return 0;
}
