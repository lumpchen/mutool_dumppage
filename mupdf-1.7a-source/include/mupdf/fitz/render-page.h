#ifndef MUPDF_FITZ_RENDER_PAGE_H
#define MUPDF_FITZ_RENDER_PAGE_H

typedef struct fz_dumped_page_s fz_dumped_page;
struct fz_dumped_page_s
{
	FILE *fp;
	void (*fz_begin_dump_fill_text)(fz_context *, fz_dumped_page *, fz_text *, const fz_matrix *, 
		fz_colorspace *, float *, float);
	void (*fz_end_dump_fill_text)(fz_context *, fz_dumped_page *);

	void (*fz_dump_char)(fz_context *, fz_dumped_page *, int, int, int, int);

	void (*fz_begin_dump_stroke_text)(fz_context *, fz_dumped_page *, fz_text *, fz_stroke_state *, 
		const fz_matrix *, fz_colorspace *, float *, float);
	void (*fz_end_dump_stroke_text)(fz_context *, fz_dumped_page *);

	void (*fz_begin_dump_fill_image)(fz_context *, fz_dumped_page *, fz_pixmap *, fz_image *, fz_matrix *);
	void (*fz_end_dump_fill_image)(fz_context *, fz_dumped_page *);

	void (*fz_begin_dump_stroke_path)(fz_context *, fz_dumped_page *, fz_path *, 
		const fz_stroke_state *, const fz_matrix *, fz_colorspace *, float *, float);
	void (*fz_end_dump_stroke_path)(fz_context *, fz_dumped_page *);

	void (*fz_begin_dump_fill_path)(fz_context *, fz_dumped_page *, fz_path *, int, const fz_matrix *, 
		fz_colorspace *, float *, float);
	void (*fz_end_dump_fill_path)(fz_context *, fz_dumped_page *);


};

fz_dumped_page *fz_new_dumped_page(fz_context *ctx, int page_number, char *filename);
void fz_drop_dumped_page(fz_context *ctx, fz_dumped_page *dumped_page);
void fz_begin_dump_fill_text(fz_context *ctx, fz_dumped_page *dumped_page, fz_text *text, const fz_matrix *ctm, 
	fz_colorspace *colorspace, float *color, float alpha);
void fz_end_dump_fill_text(fz_context *ctx, fz_dumped_page *dumped_page);

void fz_begin_dump_fill_image(fz_context *ctx, fz_dumped_page *dumped_page, fz_pixmap *pixmap, 
	fz_image *image, fz_matrix *local_ctm);
void fz_end_dump_fill_image(fz_context *ctx, fz_dumped_page *dumped_page);

void fz_dump_char(fz_context *ctx, fz_dumped_page *dumped_page, int x, int y, int gid, int ucs);

void fz_begin_dump_stroke_path(fz_context *ctx, fz_dumped_page *dumped_page, fz_path *path, 
	const fz_stroke_state *stroke, const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha);
void fz_end_dump_stroke_path(fz_context *ctx, fz_dumped_page *dumped_page);

void fz_begin_dump_fill_path(fz_context *ctx, fz_dumped_page *dumped_page, fz_path *path, int even_odd, 
	const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha);
void fz_end_dump_fill_path(fz_context *ctx, fz_dumped_page *dumped_page);

void fz_begin_dump_stroke_text(fz_context *ctx, fz_dumped_page *dumped_page, fz_text *text, fz_stroke_state *stroke,
	const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha);
void fz_end_dump_stroke_text(fz_context *ctx, fz_dumped_page *dumped_page);


#endif