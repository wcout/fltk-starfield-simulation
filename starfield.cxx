/*

 (c) 2009-2017 wcout wcout<wcout@gmx.net>

 I TRANSLATED AND ENHANCED THE FOLLOWING PROGRAM
 WRITTEN IN SMALLBASIC LANGUAGE (SMALLBASIC BY CHRISWS)
 TO C++ AND FLTK.

 THE ORIGINAL SOURCE CAN BE FOUND HERE:

 http://smallbasic.sourceforge.net/?q=node/21

 AND HERE BELOW:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'10/4/04 3:22:23 PM
'3d projection implemented in starfield with z-axis rotate
'my first ever prog made in small basic
'jelly 2004
'http://rel.betterwebber.com


'x     0           'array subscripts
'y     1
'z     2
'zvel  3         'velocity
'ox   4         'old x
'oy    5
const MAXSTARS = 200               'number of stars
const LENS = 256
const XMAX = 640
const YMAX = 480
const XMID = XMAX/2
const YMID = YMAX/2
const PI = 3.141593

dim stars(5,MAXSTARS)
dim colors(2, 255)
for i =0 to 255             'a lil hack I made to simulate 256
   colors(0, i) = i         'colors in 24 bit?
   colors(1, i) = i
   colors(2, i) = i
next i

for i = 0 to MAXSTARS
   stars(0, i) = -XMID/2 + INT(RND * XMID)
   stars(1, i) = -YMID/2 + INT(RND * YMID)
   stars(2, i) = INT(RND * LENS)
   stars(3, i) = 2 ' + INT(RND * 5)

next i

rect 0, 0, XMAX,YMAX, color 0 filled
repeat

        '///Move the stars
        '//Z=0 is 256 units away from the screen
        '//Adding values to Z moves the pixel towards us
        '//if Z > 256, the star is over our screen so reinitialize
        '//the stars Z value to 0(256 units away).
        angleZ = angleZ + 1
        rangleZ = angleZ * PI / 180            'convert to radians
        cosz = COS(rangleZ)                   'precalc
        sinz = SIN(rangleZ)

        FOR i = 0 TO MAXSTARS
            Stars(2,i) = Stars(2,i) + Stars(3,i)    'move it
            IF stars(2,i) > 255 THEN                'check for camera LENS
                stars(2,i) = 0                     'ReInit Z value
            END IF


            ox = stars(4,i)
            oy = stars(5,i)
            if ox>-1 and ox then
            if oy>-1 and oy then
               rect ox, oy, ox+1, oy+1, color 0      'erase
            end if
         end if
            tsx = Stars(0,i)                        'StarX
            tsy = Stars(1,i)                        'cleans the projectioon
            sz = Stars(2,i)                        'algo. ;*)

            sx = (tsx * cosz) - (tsy * sinz)         'Z-axis rotate
            sy = (tsy * cosz) + (tsx * sinz)


            Distance = (LENS - sz)                'get Distance

            IF Distance THEN                       'if dist>0 then
                'Projection formula
                x = XMID + (LENS * sx / Distance)
                y = YMID - (LENS * sy / Distance)
            Stars(4,i) = x
            Stars(5,i) = y
            ELSE
                                        'do nothing
                                        'you wouldn't wan't to
                                        'divide by 0 would ya? :*)

            END IF

         if x>-1 and x then
            if y>-1 and y then
               clr = (int(sz)) & 255
               r = colors(0, clr)
               g = colors(1, clr)
               b = colors(2, clr)
               c = rgb(r, g, b)
                  rect x, y, x+1, y+1, c
            end if
         end if
        NEXT i
until
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

*/

#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <cmath>
#include <cstdlib>

#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/x.H> // fl_parse_color()

class Star;
class StarField : public Fl_Double_Window
{
	typedef Fl_Double_Window Inherited;
public:
	StarField( int w_, int h_, const char *l_ = 0 );
	~StarField();
private:
	Star *stars;
	double angleZ;
	double dir;
	Fl_Widget *box;
	int numStars;
	Fl_Shared_Image *_image;
private:
	void move_stars();
	static void cb( Fl_Widget *w_, void *data_ );
	static void timer_cb( StarField *w_, void *data_ );
	virtual void draw();
};

static int MAXSTARS = 256;
static const int LENS = 256;
static const double PI = 3.141592654;
static const double FPS = 1. / 25.;
static const Fl_Color BGCOLOR = FL_BLACK;
static const Fl_Color FGCOLOR = FL_WHITE;
static const int MAXSTARSIZE = 6;
static char *StarImageName = 0;
static double ZoomFactor = 1. / 3;
static int midx = -1;
static int midy = -1;

class Star
{
public:
	int x;          // init. x-pos
	int y;          // init. y-pos
	int z;          // current z-pos
	int speed;      // z increment
	int size;       // size of star px
	int dx;         // draw x-pos
	int dy;         // draw y-pos
	int color;      // draw color
};

StarField::StarField( int w_, int h_, const char *l_ ) :
	Inherited( w_, h_ ),
	stars( 0 ),
	angleZ( 0 ),
	dir( 0.5 ),     // rotate by 0.5 degree / frame
	numStars( MAXSTARS ),
	_image( 0 )
{
	color( Fl::get_color( FL_BACKGROUND_COLOR ) );
	if ( StarImageName )
	{
		_image = Fl_Shared_Image::get( StarImageName );
		if ( _image->w() <= 0 || _image->h() <= 0 )
		{
			_image->release();
			_image = 0;
		}
		else if ( MAXSTARS == 256 )
		{
			numStars = 16;
		}
	}
	char title[100];
	snprintf( title, sizeof( title ), "%s (%d %s)",
		l_, numStars, (_image ? StarImageName : "stars" ) );
	copy_label( title );
	box = new Fl_Button( 0, 0, w(), h() );
	box->box( FL_NO_BOX );
	box->visible_focus( 0 );
	int xmid = w() / 2;
	int ymid = h() / 2;
	stars = new Star[ numStars ];
	for ( int i = 0; i < numStars; i++ )
	{
		stars[i].x = -xmid / 2 + random() % xmid;
		stars[i].y = -ymid / 2 + random() % ymid;
		stars[i].z = random() % LENS;
		stars[i].speed = 2; //+ random()%5;
		stars[i].size = random() % MAXSTARSIZE + 1;
		stars[i].dx = stars[i].x;
		stars[i].dy = stars[i].y;
		stars[i].color = stars[i].z;
	}
	Fl::add_timeout( FPS, (Fl_Timeout_Handler)timer_cb, (void *)this );
	box->callback( cb, this );
}

StarField::~StarField()
{
	delete[] stars;
	_image->release();
}

/*static*/
void StarField::cb( Fl_Widget *w_, void *data_ )
{
	StarField *w = (StarField *)data_;
	if ( Fl::event_clicks() )
	{
		// Double click - toggle fullscreen and rotation
		w->dir = -w->dir;
		if ( w->dir <  0 )
			w->fullscreen();
		else
			w->fullscreen_off( 100, 100, 800, 600 );
		Fl::event_is_click( 0 );
		midx = -1;
		midy = -1;
	}
	else
	{
		// Single click - set center of movement
		midx = Fl::event_x();
		midy = Fl::event_y();
	}
} // cb

/*virtual*/
void StarField::draw()
{
	Inherited::draw();
	for ( int z = 0; z < LENS; z++ )
	{
		for ( int i = 0; i < numStars; i++ )
		{
			if ( stars[i].z != z )
				continue;
			if ( _image )
			{
				int sz = (float)( stars[i].z + 10 ) * ZoomFactor;
				Fl_Shared_Image *image = Fl_Shared_Image::find( _image->name(), sz, sz );
				if ( !image )
					image = _image->get( _image->name(), sz, sz );
				Fl_Image *temp = image->copy( image->w(), image->h() );
				temp->color_average( color(), float( stars[i].z  + 1 )/ (float)LENS );
				temp->draw( stars[i].dx - temp->w() / 2, stars[i].dy - temp->h() / 2 );
				delete temp;
			}
			else
			{
				Fl_Color c = fl_color_average( labelcolor(), color(), float( stars[i].color ) / LENS );
				fl_color( c );
				fl_pie( stars[i].dx, stars[i].dy, stars[i].size, stars[i].size, 0., 360. );
			}
		}
	}
} // draw

void StarField::move_stars()
{
	angleZ += dir;
	double rangleZ = angleZ * PI / 180.0;
	double cosz = cos( rangleZ );
	double sinz = sin( rangleZ );
	int xmid = midx >= 0 ? midx : w() / 2;
	int ymid = midx >= 0 ? midy : h() / 2;
	for ( int i = 0; i < numStars; i++ )
	{
		stars[i].z += stars[i].speed;
		if ( stars[i].z >= LENS )
			stars[i].z = 0;
		double tsx = stars[i].x;
		double tsy = stars[i].y;
		int sz = stars[i].z;
		double sx = ( tsx * cosz ) - ( tsy * sinz );
		double sy = ( tsy * cosz ) + ( tsx * sinz );
		int distance = LENS - sz;
		if ( distance )
		{
			stars[i].dx = xmid + (int)( (double)LENS * ( sx / (double)distance ) );
			stars[i].dy = ymid - (int)( (double)LENS * ( sy / (double)distance ) );
			stars[i].color = sz;
		}
	}
} // move_stars

/*static*/
void StarField::timer_cb( StarField *w_, void *data_ )
{
	w_->redraw();
	Fl::repeat_timeout( FPS, (Fl_Timeout_Handler)timer_cb, w_ );
	w_->move_stars();
}

int main( int argc, char **argv )
{
	fl_register_images();
	Fl::visual( FL_RGB );
	Fl::get_system_colors();
	// allow setting bg and fg color via commandline
	// mimicks Fl::args() for -bg and -fg.
	Fl_Color bgcolor( BGCOLOR );
	Fl_Color fgcolor( FGCOLOR );
	uchar r, g, b;
	for ( int i = 1; i < argc; i++ )
	{
		// allow setting an image for a star
		if ( argv[i][0] != '-' )
			StarImageName = argv[i];
		else
		{
			if ( i + 1 >= argc )
				break;
			i++;
			// allow changing number of stars
			if ( strcmp( &argv[i-1][1], "n" ) == 0 )
			{
				MAXSTARS = atoi( argv[i] );
				continue;
			}
			// allow changing number of stars
			else if ( strcmp( &argv[i-1][1], "z" ) == 0 )
			{
				ZoomFactor = atof( argv[i] );
				if ( ZoomFactor <= 0. || ZoomFactor > 10 )
					ZoomFactor = 1.;
				continue;
			}
			// parse color value 'name' or '#rrggbb'
			int ok = fl_parse_color( argv[i], r, g, b );
			if ( !ok )
				continue;
			if ( strcmp( &argv[i-1][1], "bg" ) == 0 )
				bgcolor = fl_rgb_color( r, g, b );
			else if ( strcmp( &argv[i-1][1], "fg" )  == 0 )
				fgcolor = fl_rgb_color( r, g, b );
		}
	}
	Fl_Double_Window *window = new StarField( 800, 600, "starfield" );
	window->color( bgcolor );
	window->labelcolor( fgcolor );
	window->resizable( window );
	window->show();
	return Fl::run();
}
