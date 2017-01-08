/*

 Starfield simulation in FLTK.

 (c) 2009-2017 wcout wcout<wcout@gmx.net>

 I translated and enhanced a program written in
 SmallBasic language (SmallBasic by ChrisWS)
 to C++ and FLTK.

 I found this code about 2009 "in the web", transcoded
 it and have improved it slowly over the years, then
 forgot about it for some more years and now (2017)
 have found it again and decided to make it public.

 As there is no explicit claim for non free code in
 the original source, I "assume" it can be modified
 and redistributed freely.

 Should this assumption be wrong, please mail me
 and I will remove it from public access.

 This translated code can be used and modified freely
 without any restrictions **in the assumption the original
 code is free too**.

 The original source can be found under this link:

    http://smallbasic.sourceforge.net/?q=node/21

 and is also pasted here below:

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
#include <FL/fl_draw.H>
#include <cmath>
#include <cstdlib>

#include <FL/Fl_Shared_Image.H>
#include <FL/x.H> // fl_parse_color()

class Star;
class StarField : public Fl_Double_Window
{
	typedef Fl_Double_Window Inherited;
public:
	StarField( int w_, int h_, const char *l_ = 0 );
	~StarField();
	virtual int handle( int e_ );
private:
	Star *_stars;              // array of 'stars'
	double _angleZ;            // current rotation angle
	double _dir;               // rotation increment
	int _numStars;             // number of starts in array
	Fl_Shared_Image *_image;
	int _midx;                 // current x center of movement
	int _midy;                 // current y center of movement
private:
	void move_stars();
	static void timer_cb( StarField *w_, void *data_ );
	virtual void draw();
};

// currently fixed values
static const int LENS = 256;
static const int MAX_STAR_SIZE = 6;

// command line changeable values
static Fl_Color BgColor = FL_BLACK;
static Fl_Color FgColor = FL_WHITE;
static char *StarImageName = 0;
static double ZoomFactor = 1. / 3;
static int MaxStars = 256;
static double Fps = 1. / 25.;

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
	_stars( 0 ),
	_angleZ( 0 ),
	_dir( 0.5 ),     // rotate by 0.5 degree / frame
	_numStars( MaxStars ),
	_image( 0 ),
	_midx( -1 ),
	_midy( -1 )
{
	if ( StarImageName )
	{
		_image = Fl_Shared_Image::get( StarImageName );
		if ( _image->w() <= 0 || _image->h() <= 0 )
		{
			_image->release();
			_image = 0;
		}
		else if ( MaxStars == 256 )
		{
			_numStars = 16;
		}
	}
	char title[100];
	snprintf( title, sizeof( title ), "%s (%d %s)",
		l_, _numStars, (_image ? StarImageName : "stars" ) );
	copy_label( title );
	int xmid = w() / 2;
	int ymid = h() / 2;
	_stars = new Star[ _numStars ];
	for ( int i = 0; i < _numStars; i++ )
	{
		_stars[i].x = -xmid / 2 + random() % xmid;
		_stars[i].y = -ymid / 2 + random() % ymid;
		_stars[i].z = random() % LENS;
		// Note: currently same speed for all stars, because it is "nicer" to watch
		_stars[i].speed = 2; //+ random() % 2;
		_stars[i].size = random() % MAX_STAR_SIZE + 1;
		_stars[i].dx = _stars[i].x;
		_stars[i].dy = _stars[i].y;
		_stars[i].color = _stars[i].z;
	}
	move_stars();
	Fl::add_timeout( Fps, (Fl_Timeout_Handler)timer_cb, (void *)this );
}

StarField::~StarField()
{
	delete[] _stars;
	_image->release();
}

/*virtual*/
int StarField::handle( int e_ )
{
	int ret = Inherited::handle( e_ );
	if ( FL_PUSH != e_ )
		return ret;

	// handle mouse click
	if ( Fl::event_clicks() )
	{
		// Double click - toggle fullscreen and rotation
		_dir = -_dir;
		if ( fullscreen_active() )
			fullscreen_off( 100, 100, 800, 600 );
		else
			fullscreen();
		Fl::event_is_click( 0 );
		_midx = -1;
		_midy = -1;
	}
	else
	{
		// Single click - set center of movement
		_midx = Fl::event_x();
		_midy = Fl::event_y();
	}
	return ret;
} // handle

/*virtual*/
void StarField::draw()
{
	Inherited::draw();
	for ( int z = 0; z < LENS; z++ )
	{
		for ( int i = 0; i < _numStars; i++ )
		{
			// draw one distance layer after the other..
			if ( _stars[i].z != z )
				continue;
			if ( _image )
			{
				// draw star images
				int sz = (float)( _stars[i].z + 10 ) * ZoomFactor;
				// This should cache all needed image sizes to speed up drawing
				Fl_Shared_Image *image = Fl_Shared_Image::find( _image->name(), sz, sz );
				if ( !image )
					image = _image->get( _image->name(), sz, sz );
				// Just for color averaging we still need a temporary 1:1 copy..
				Fl_Image *temp = image->copy( image->w(), image->h() );
				temp->color_average( color(), float( _stars[i].z  + 1 )/ (float)LENS );
				temp->draw( _stars[i].dx - temp->w() / 2, _stars[i].dy - temp->h() / 2 );
				delete temp;
			}
			else
			{
				// draw star 'dots'
				Fl_Color c = fl_color_average( labelcolor(), color(), float( _stars[i].color ) / LENS );
				fl_color( c );
				fl_pie( _stars[i].dx, _stars[i].dy, _stars[i].size, _stars[i].size, 0., 360. );
			}
		}
	}
} // draw

void StarField::move_stars()
{
	_angleZ += _dir;
	double rangleZ = _angleZ * M_PI / 180.0;
	double cosz = cos( rangleZ );
	double sinz = sin( rangleZ );
	int xmid = _midx >= 0 ? _midx : w() / 2;
	int ymid = _midx >= 0 ? _midy : h() / 2;
	for ( int i = 0; i < _numStars; i++ )
	{
		_stars[i].z += _stars[i].speed;
		if ( _stars[i].z >= LENS )
			_stars[i].z = 0;
		double tsx = _stars[i].x;
		double tsy = _stars[i].y;
		int sz = _stars[i].z;
		double sx = ( tsx * cosz ) - ( tsy * sinz );
		double sy = ( tsy * cosz ) + ( tsx * sinz );
		int distance = LENS - sz;
		if ( distance )
		{
			_stars[i].dx = xmid + (int)( (double)LENS * ( sx / (double)distance ) );
			_stars[i].dy = ymid - (int)( (double)LENS * ( sy / (double)distance ) );
			_stars[i].color = sz;
		}
	}
} // move_stars

/*static*/
void StarField::timer_cb( StarField *w_, void *data_ )
{
	w_->redraw();
	Fl::repeat_timeout( Fps, (Fl_Timeout_Handler)timer_cb, w_ );
	w_->move_stars();
}

int main( int argc, char **argv )
{
	fl_register_images();
	Fl::visual( FL_RGB );
	Fl::get_system_colors();
	// allow setting bg and fg color via commandline
	// mimicks Fl::args() for -bg and -fg.
	uchar r, g, b;
	bool fullscreen( false );
	for ( int i = 1; i < argc; i++ )
	{
		// allow setting an image for a star
		if ( argv[i][0] != '-' )
			StarImageName = argv[i];
		else
		{
			// allow fullscreen
			if ( strcmp( &argv[i][1], "f" ) == 0 )
			{
				fullscreen = true;
				continue;
			}
			if ( i + 1 >= argc )
				break;
			i++;
			// allow changing number of stars
			if ( strcmp( &argv[i-1][1], "n" ) == 0 )
			{
				MaxStars = atoi( argv[i] );
				continue;
			}
			// allow changing speed
			if ( strcmp( &argv[i-1][1], "s" ) == 0 )
			{
				Fps = atof( argv[i] );
				if ( Fps < 0.001 || Fps > 0.5 )
					Fps = 1. / 25;
				continue;
			}
			// allow changing zoom of star image
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
				BgColor = fl_rgb_color( r, g, b );
			else if ( strcmp( &argv[i-1][1], "fg" )  == 0 )
				FgColor = fl_rgb_color( r, g, b );
		}
	}
	Fl_Double_Window *window = new StarField( 800, 600, "starfield" );
	window->color( BgColor );
	window->labelcolor( FgColor );
	window->resizable( window );
	window->show();
	if ( fullscreen )
		window->fullscreen();
	return Fl::run();
}
