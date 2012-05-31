/*
 * License: MIT/X Consortium License. For more details, see the LICENSE file
 *
 * This application demonstrates the use of libcensure. libcensure is an
 * implementation of the Center Surround Extremas [1].
 *
 * [1] CenSurE: Center Surround Extremas for Realtime Feature Detection and
 *     Matching; Motilal Agrawal, Kurt Nonolige, et. al; Computer Vision - ECCV
 *     2008; ISBN 978-3-540-88692-1
 */
#include <libcensure.h>
#include <iostream>
#include <cstdlib>
#include <cv.h>
#include <highgui.h>

#define ESC       27
//#define CAM_W     480
//#define CAM_H     320
#define CAM_W    352
#define CAM_H    288
#define WIN_NAME "censure_demo"

#define COL_NEW         CV_RGB(0, 0, 255)
#define COL_TRACKED     CV_RGB(0, 255, 0)
#define COL_MOTION_LINE CV_RGB(255, 0, 0)


// callback function to extract the pixel coordinate value from an image
unsigned getimgdata_fn(void *p, unsigned x, unsigned y)
{
	CvScalar s = cvGet2D((IplImage*)p, y, x);
	return (unsigned)s.val[0];
}


inline void
swap(short * x, short * y)
{
	short tmp = *x;
	*x = *y;
	*y = tmp;
}


void
cv_mark_feature(IplImage *img, struct csx_feature *f, int radius,
	CvScalar col)
{
	CvPoint center = cvPoint(f->x, f->y);
	cvCircle(img, center, radius, col, 1, 8, 0);
	cvLine(img, center, cvPoint(f->x, f->y - radius), col, 1, 8, 0);
}


void
cv_mark_features( IplImage *img, struct csx_features *fs1,
	struct csx_features *fs2, struct csx_match_table *mtable, int radius,
	int *extents)
{
	int _radius = radius;
	for (size_t i = 0; i < fs1->n; i++) {
		struct csx_feature *f = &fs1->f[i];
		if (!radius)
			_radius = extents[f->scale];

		if (f->mtable_id >= 0 && mtable && fs2) {
			struct csx_feature *f2 = &fs2->f[mtable->r[f->mtable_id]];

			cv_mark_feature(img, f, _radius, COL_TRACKED);
			cvLine(img, cvPoint(f->x, f->y), cvPoint(f2->x, f2->y),
				COL_MOTION_LINE, 1, 8, 0);

		}
		else
			cv_mark_feature(img, f, _radius, COL_NEW);
	}
}


int
main ()
{
	// set up libcensure. You have the choice between OCT and BOX filters
	struct csx_setup *setup = NULL;
	if (csx_initialize(CSX_FT_OCT, CAM_W, CAM_H, &setup)) {
		std::cerr << "Could not initialize libcensure." << std::endl;
		return EXIT_FAILURE;
	}

	// capture from first available webcam
	CvCapture *cam = cvCaptureFromCAM(0);
	if (!cam) {
		std::cerr << "Could not open capture device." << std::endl;
		return EXIT_FAILURE;
	}
	cvSetCaptureProperty(cam, CV_CAP_PROP_FRAME_WIDTH, CAM_W);
	cvSetCaptureProperty(cam, CV_CAP_PROP_FRAME_HEIGHT, CAM_H);

	// display the live-tracking results. You can exit the application by
	// hitting the ESC key. libcensure currently supports grayscales images
	// only, thus it is necessary to convert the capture frame
	int result = EXIT_SUCCESS;
	IplImage *rgb = NULL;
	IplImage *gray = NULL;
	csx_detection_result *dr[2] = {NULL, NULL};
	csx_match_table *mt = NULL;

	short CURRENT = 0;
	short PAST = 1;

	cvNamedWindow(WIN_NAME, CV_WINDOW_AUTOSIZE);
	for (;;) {

		// grab frame and convert to grayscale
		rgb = cvQueryFrame(cam);
		if (!rgb) {
			std::cerr << "Could not retrieve frame." << std::endl;
			result = EXIT_FAILURE;
			break;
		}
		if (!gray) gray = cvCreateImage(cvSize(rgb->width, rgb->height), IPL_DEPTH_8U, 1);
		cvCvtColor(rgb, gray, CV_RGB2GRAY);

		// detect features in this frame. this will create surf64
		// descriptors for each feature as well
		if (csx_detect(setup, getimgdata_fn, gray, &dr[CURRENT])) {
			std::cerr << "CenSurE feature detection failed." << std::endl;
			result = EXIT_FAILURE;
			break;
		}

		// track features between this and the last frame
		csx_track(dr[CURRENT], dr[PAST], &mt);

		// mark the features on the image
		if (dr[PAST])
			cv_mark_features(rgb, dr[CURRENT]->fs, dr[PAST]->fs, mt, 0, dr[CURRENT]->fparams->margins);
		else
			cv_mark_features(rgb, dr[CURRENT]->fs, NULL, NULL, 0, dr[CURRENT]->fparams->margins);

		// display results
		cvShowImage(WIN_NAME, rgb);

		// swap the data for last and current frame simply by exchanging
		// the indices
		swap(&CURRENT, &PAST);

		if ((cvWaitKey(10) & 255) == ESC) break;
	}

	// release memory acquired by library
	csx_free_match_table(&mt);
	csx_free_detection_result(&dr[CURRENT]);
	csx_free_detection_result(&dr[PAST]);
	csx_finalize(&setup);

	if (rgb) cvReleaseImage(&rgb);
	if (gray) cvReleaseImage(&gray);
	cvReleaseCapture(&cam);
	return result;
}
