#include <GL/glut.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <highgui.h>
#include <ctype.h>
#include <fcntl.h>
#include <cv.h>

#define SIZE 150      	/*Rendering Quality*/
#define HEADER 15     	/*number of characters in .pbm header 
			"P1\n#width #height\n255\n" = 9 + 2*(SIZE's length)*/
#define TRR 4.0/SIZE 	/*Vortex arrest size*/
#define max(a, b) ((a)>(b)?(a):(b))
#define min(a, b) ((a)<(b)?(a):(b))


//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------

// Saving related
char std_dst1[]="0pic";
char std_dst2[]="0data";
char ext1[]=".ppm";
char ext2[]=".pbm";

// Image-Editing related
IplImage *src_img = 0, *dst_img = 0;
CvFont font;
int t_quit, 	/*trigger for quitting*/
t_skip_cap=0, 	/*counter for skipping capture*/
t_data=0, 	/*trigger for loading from "0data.pbm"-like files*/
t_mode=0;	/*edit mode indicator*/
int header;	/*returns error when not equal to HEADER*/
int c;		/*Keyboard Input*/
int x0_pos=500, /*Initial Window Position*/
y0_pos=200; 	


// 3d-Modeling related
// (Matrix)
unsigned char front[SIZE*SIZE+HEADER], right[SIZE*SIZE+HEADER], 
up[SIZE*SIZE+HEADER];	
static const int tt_size = SIZE*SIZE+HEADER;	/*Input .bpm File's length*/
unsigned int output[SIZE][SIZE][SIZE], rendering[SIZE][SIZE][SIZE];

// (Point of View)
static double distance = 5.0, pitch = 0.0, yaw = 0.0;

// (Mouse)
GLint mouse_button = -1;
GLint mouse_x = 0, mouse_y = 0;

// (Light & Material)
static const GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
static const GLfloat light_ambient[] = {0.4, 0.4, 0.4, 1.0};
static const GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
static const GLfloat mat_default_color[] = { 1.0, 0.0, 0.0, 1.0 };
static const GLfloat mat_default_specular[] = { 1.0, 1.0, 1.0, 1.0 };
static const GLfloat mat_default_shininess[] = { 100.0 };
static const GLfloat mat_default_emission[] = {0.0, 0.0, 0.0, 0.0};



         ////////////////////////////////////////////////////////////////////
         //
         //   Capture & Editing related
         //
         ////////////////////////////////////////////////////////////////////


//----------------------------------------------------------------------------
// Check HEADER's value
//----------------------------------------------------------------------------

void check_header (void)
{
  char head[]="";
  int size_len, header;
  sprintf(head,"%d",SIZE);
  size_len = strlen(head);
  header = 9+2*size_len;
  if(header != HEADER){
    printf("\n");
    perror("HEADER_VALUE ERROR");
    printf("Please update HEADER's value to %d\n\n",header);
    t_quit=1;
    return;
  }
}


//----------------------------------------------------------------------------
// Capture
//----------------------------------------------------------------------------

void capture_to (int num)
{
  CvCapture *capture = 0;
  IplImage *frame = 0;
  int i, pos;
  int width, height;
  int cross_size = 5;			/*center cross size*/
  CvPoint grid_point[5][2][2];		/*grid's border points*/
  CvPoint cross[4]; 			/*center cross's points*/
  char buffer_num[10];
  char buffer_txt[100];

  // (1) Initialization

  // (Saving-related)
  sprintf(buffer_num,"%d",num);
  strcpy(buffer_txt,std_dst1);
  strcat(strcat(buffer_txt,buffer_num), ext1);

  // (Create Camera)
  capture = cvCreateCameraCapture(0);

  // (Window size & cross' points)
  width = cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_WIDTH);
  height = cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_HEIGHT);
  int min_size = min(width,height);
  int center = min_size/2;
  CvRect sqr_ROI = cvRect((width/2 - center), (height/2 -center), min_size,min_size);	/*ROI for square-sized capture*/
  cross[0] = cvPoint(center-cross_size, center);
  cross[1] = cvPoint(center+cross_size, center);
  cross[2] = cvPoint(center, center-cross_size);
  cross[3] = cvPoint(center, center+cross_size);
  
  // (Centralize Window)
if(num==1){
#ifdef __linux__
    char *command="xrandr | grep '*'";
    FILE *fpipe = (FILE*)popen(command,"r");
    char line[256];
    int monitor_width, monitor_height;
    char x[]="x";
    char *token;
    fgets( line, sizeof(line), fpipe);
    token = strtok(line,x);
    monitor_width=atoi(token);
    token=strtok(NULL,x);
    monitor_height=atoi(token);
    pclose(fpipe);
    x0_pos = monitor_width/2 - center;
    y0_pos = monitor_height/2 - center;
#endif
  }

  // (Create Window)
  cvNamedWindow ("Capture", CV_WINDOW_AUTOSIZE);
  cvMoveWindow("Capture", x0_pos, y0_pos);

  // (Set values)
  c = 0;
  t_quit = 0;
  
  // (2) Capture Loop
  while (1) {

    // (a) Start capture & resize
    frame = cvQueryFrame (capture);
    cvFlip(frame,0,1);
    cvSetImageROI(frame, sqr_ROI);  

    // (b) Keyboard callback
    c = cvWaitKey (1);
    if (c == 27 /*Escape key*/){
      t_quit = 1;
      break;
    }
    if (c == 32 /*Space key*/) {
      t_skip_cap+=1;
      break;
    }
    if (c == 10 /*Enter key*/){
      cvSaveImage(buffer_txt,frame,0);
      printf("\n*************************\n");
      printf("Saved to %s\n",buffer_txt);
      printf("*************************\n\n");
      break;
    }
    
    // (c) Drawing Grid & Cross
    for(i=0;i<5;i++){
      pos = (i+1)*min_size/6;
      grid_point[i][0][0]=cvPoint(pos, 0);
      grid_point[i][1][0]=cvPoint(pos, min_size);
      grid_point[i][0][1]=cvPoint(0, pos);
      grid_point[i][1][1]=cvPoint(min_size,pos);
      cvLine(frame,grid_point[i][0][0],grid_point[i][1][0],CV_RGB(170,170,170),1,CV_AA,0);  
      cvLine(frame,grid_point[i][0][1],grid_point[i][1][1],CV_RGB(170,170,170),1,CV_AA,0);  
  }
    cvLine(frame,cross[0],cross[1],CV_RGB(0,255,0),2,CV_AA,0);     
    cvLine(frame,cross[2],cross[3],CV_RGB(0,255,0),2,CV_AA,0);

    // (d) Show Image
    cvShowImage ("Capture", frame);
    cvResetImageROI(frame);
  }

  cvReleaseCapture (&capture);
  cvDestroyWindow ("Capture");
}


//----------------------------------------------------------------------------
// Editing
//----------------------------------------------------------------------------

void on_trackbar (int val);

void threshold_to (int num, char** argv, int mode)
{
  int i;
  IplImage *background_img = 0;
  char buffer_num[10];
  char buffer_src[100];		/*Load from*/
  char buffer_dst[100];		/*Save to*/

  // (1) Initialization

  // (Saving & Loading-related)
  sprintf(buffer_num,"%d",num);
  strcpy(buffer_dst,std_dst2);  
  strcat(strcat(buffer_dst,buffer_num), ext2);
  if(mode) strcpy(buffer_src,argv[num+1]);
  else {
    strcpy(buffer_src,std_dst1);
    strcat(strcat(buffer_src,buffer_num), ext1);
  }
  
  // (Create font)
  cvInitFont (&font, CV_FONT_HERSHEY_DUPLEX, 1.0, 1.0, 0.0, 1, CV_AA);

  // (Set values)
  c = 0;
  t_quit = 1;

  // (2) Loading
  if( (src_img = cvLoadImage (buffer_src, CV_LOAD_IMAGE_GRAYSCALE)) == 0) {
    perror("IMAGE_LOAD ERROR");
    return;
  }
  
  // (3) Resize to Square
  if(mode){
      int width, height;
      width = src_img->width;
      height= src_img->height;
      int new_size = max(width,height);
      IplImage *tmp;
      tmp = cvCreateImage(cvSize(new_size,new_size) ,src_img->depth, src_img->nChannels);
      cvRectangle (tmp, cvPoint (0, 0), cvPoint (new_size, new_size), cvScalar (255, 255, 255, 1), CV_FILLED, 8, 0);
      CvRect putimg_ROI = cvRect((new_size-width)/2,(new_size-height)/2,width,height);
      cvSetImageROI(tmp, putimg_ROI);
      cvCopyImage(src_img,tmp);
      cvResetImageROI(tmp);
      cvReleaseImage (&src_img);
      src_img = cvCloneImage(tmp);
      cvReleaseImage (&tmp);
  }

  // (4) Create Images
  dst_img = cvCreateImage (cvGetSize (src_img), IPL_DEPTH_8U, 1);
  background_img = cvCloneImage(src_img);

  // (2) Create Window & Trackbar
  cvNamedWindow ("Editing", CV_WINDOW_AUTOSIZE);
  cvMoveWindow("Editing", (150+x0_pos)/2, (100+y0_pos)/2);
  cvCreateTrackbar ("Trackbar", "Editing", 0, 255, on_trackbar);

  // (3) Show Image
  char *str[]= {"Esc key.....Exit",
		"Enter key...Save Picture"};
  for(i=0;i<2;i++) {
    cvPutText (background_img, str[i], cvPoint (16, 41+40*i), &font, CV_RGB (255, 255, 255));
    cvPutText (background_img, str[i], cvPoint (15, 40+40*i), &font, CV_RGB (0, 0, 0));
  }
  cvShowImage ("Editing", background_img);

  // (4) Keyboard callback
  while(1){
    c = cvWaitKey(0);
    switch(c){
    case 10 /*Enter key*/:
      cvSaveImage(buffer_dst,dst_img,0);
      printf("\n*************************\n");
      printf("Saved to %s\n",buffer_dst);
      printf("*************************\n\n");
      t_quit=0;
    case -1 /*Close Window*/:
    case 27 /*Escape key*/:
      return;
    }
  }

  cvDestroyWindow ("Editing");
  cvReleaseImage (&src_img);
  cvReleaseImage (&dst_img);
  cvReleaseImage (&background_img);
}

// (5) Trackbar callback
void on_trackbar (int val)
{
  switch (c){
  case 65361: //←
    val-=1;
    break;
  case 65363: //→
    val+=1;
    break;
  }

  cvThreshold (src_img, dst_img, val, 255, CV_THRESH_BINARY);
  cvShowImage ("Editing", dst_img);
}


         ////////////////////////////////////////////////////////////////////
         //
         //   3D-Modeling related
         //
         ////////////////////////////////////////////////////////////////////


//----------------------------------------------------------------------------
// Initialize
//----------------------------------------------------------------------------

void init(void)
{
  // Clear
  glClearColor (0.0, 0.0, 0.0, 0.0);
  glClearDepth( 1.0 );

  // Depth
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LESS );
  glShadeModel (GL_SMOOTH);

  // Light
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_ambient);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  // Material
  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_default_color);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_default_color);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_default_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_default_shininess);

}


//----------------------------------------------------------------------------
// Load Images
//----------------------------------------------------------------------------

void LoadImg(int argc, char** argv){
  IplImage *src_img[3] = {0,0,0}, *dst_img[3] = {0,0,0};
  int i;

   // (1) Loading
   for(i=0;i<argc-1;i++){
     if(( src_img[i] = cvLoadImage (argv[i+1], CV_LOAD_IMAGE_GRAYSCALE)) == 0){
       perror("IMAGE_LOAD ERROR");
       t_quit = 1;
       return;
     }
   }
   for(i=argc-1;i<3;i++) 
     if(( src_img[i] = cvLoadImage ("00.pbm", CV_LOAD_IMAGE_GRAYSCALE)) == 0){
       perror("IMAGE_LOAD ERROR");
       printf("Could not find '00.pbm'\n");
       t_quit = 1;
       return;
     }
   
   // (2) Resize & Invert Colors
   for(i=0;i<3;i++){
     dst_img[i] = cvCreateImage (cvSize(SIZE,SIZE), IPL_DEPTH_8U, 1);
     cvResize (src_img[i], dst_img[i], CV_INTER_LINEAR);
     cvThreshold (dst_img[i], dst_img[i], 200, 255, CV_THRESH_BINARY_INV);

   // (3) Saving
     if(i==0) cvSaveImage("00f.pbm",dst_img[0], 0);
     if(i==1) cvSaveImage("00r.pbm",dst_img[1], 0);
     if(i==2) cvSaveImage("00u.pbm",dst_img[2], 0);
   }
   
   for(i=0;i<3;i++){
     cvReleaseImage (&dst_img[i]);
     cvReleaseImage (&src_img[i]);
   }
}


//----------------------------------------------------------------------------
// Compute Matrix
//----------------------------------------------------------------------------

void GenMatrix() {
  int tf,tr,tu;		/*trigger for Front, Right and Up*/
  int fd_pbm;		/*file descriptor*/
  int i,j,k;
  
  // (1) Generate base (front; right; up)
  fd_pbm = open("00f.pbm", O_RDWR, 0644);
  read(fd_pbm, front, tt_size); 
  close(fd_pbm);

  fd_pbm = open("00r.pbm", O_RDWR, 0644);
  read(fd_pbm, right, tt_size); 
  close(fd_pbm);

  fd_pbm = open("00u.pbm", O_RDWR, 0644);
  read(fd_pbm, up, tt_size); 
  close(fd_pbm);

  // (2) Initialize output & rendering
#pragma omp parallel for
  for(i=HEADER;i<tt_size;i++){
#pragma omp parallel for
      for(k=0;k<SIZE;k++){
        output[(i-HEADER)/SIZE][(i-HEADER)%SIZE][k]=1;
	rendering[(i-HEADER)/SIZE][(i-HEADER)%SIZE][k]=0;	
      }
  }

  // (3) Computate output Matrix
  // (Space Carving Method)

#pragma omp parallel for private(i,tf,tr,tu)
  for(i=HEADER;i<tt_size;i++){
      tf=0;
      tr=0;
      tu=0;
      if((int)front[i]==0) tf=1;
      if((int)right[i]==0) tr=1;
      if((int)up[i]  == 0) tu=1;
#pragma omp parallel for
      for(k=0;k<SIZE;k++){
	if(tf) output[(i-HEADER)/SIZE][(i-HEADER)%SIZE][k]=0;
	if(tr) output[(i-HEADER)/SIZE][k][SIZE-1-(i-HEADER)%SIZE]=0;
	if(tu) output[k][(i-HEADER)%SIZE][(i-HEADER)/SIZE]=0;
      }
  }	

  // (4) Computate rendering Matrix
  // (Detect outer voxels)

#pragma omp parallel for
  for(i=1;i<SIZE-1;i++){
#pragma omp parallel for
    for(j=1;j<SIZE-1;j++){
#pragma omp parallel for
      for(k=1;k<SIZE-1;k++){
	if(
	   (output[i][j][k]) &&
	   !((output[i-1][j][k]) &&
	     (output[i+1][j][k]) && 
	     (output[i][j-1][k]) && 
	     (output[i][j+1][k]) && 
	     (output[i][j][k-1]) && 
	     (output[i][j][k+1]))
	   ) rendering[i][j][k] = 1;
      }
    }
  }
}


//----------------------------------------------------------------------------
// drawBox
//----------------------------------------------------------------------------

void drawBox(void)
{
  int i,j,k;
  glTranslatef(-TRR*SIZE/2, TRR*SIZE/2, -TRR*SIZE/2);
  for(i=0;i<SIZE;i++){
    for(j=0;j<SIZE;j++){
      for(k=0;k<SIZE;k++){	
	if(rendering[i][j][k]){
	  glTranslatef(j*TRR, -i*TRR, k*TRR);
	  glutSolidCube(TRR);
	  glTranslatef(-j*TRR, i*TRR, -k*TRR);
	}
      }
    }
  }
}


//----------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------

void display(void)
{
  // (1) Clear Frame
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // (2) Set Viewpoint
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, 0.0, distance, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  glRotatef( -pitch, 1.0, 0.0, 0.0 );
  glRotatef( -yaw, 0.0, 1.0, 0.0 );

  // (3) Draw
  drawBox();

  glutSwapBuffers();
}


//----------------------------------------------------------------------------
// Callbacks
//----------------------------------------------------------------------------

// (Window)
void reshape (int w, int h)
{
  glViewport (0, 0, (GLsizei) w, (GLsizei) h); 

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective(60.0, (GLfloat) w/(GLfloat) h, 1.0, 20.0);

}

// (Keyboard)
void keyboard (unsigned char key, int x, int y)
{
  switch (key) {
  case 27://ESCで終了
    exit(0);
    break;
  }
}

// (Mouse)
void mouse(int button, int state, int x, int y)
{
  mouse_button = button;
  mouse_x = x;	mouse_y = y;

  if(state == GLUT_UP){
    mouse_button = -1;
  }

  glutPostRedisplay();
}

//(Mouse Movement)
void motion(int x, int y)
{
  switch(mouse_button){
  case GLUT_LEFT_BUTTON: //Move

    if( x == mouse_x && y == mouse_y )
      return;

    yaw -= (GLfloat) (x - mouse_x) / 2.5;
    pitch -= (GLfloat) (y - mouse_y) / 2.5;

    break;

  case GLUT_RIGHT_BUTTON: //Zoom

    if( y == mouse_y )
      return;

    if( y < mouse_y )
      distance += (GLfloat) (mouse_y - y)/50.0;
    else
      distance -= (GLfloat) (y - mouse_y)/50.0;

    if( distance < 1.0 ) distance = 1.0;
    if( distance > 10.0 ) distance = 10.0;

    break;
  }

  mouse_x = x;
  mouse_y = y;

  glutPostRedisplay();
}

// (Idle)
void idle()
{
  glutPostRedisplay();
}


         ////////////////////////////////////////////////////////////////////
         //
         //   Main Function
         //
         ////////////////////////////////////////////////////////////////////


int main(int argc, char** argv)
{
  int i;
  check_header();
  if(t_quit) return 1;

  // Capture Mode
  if(argc == 1)
    {
      t_data = 1;
      for(i=1;i<=3;i++) {
	capture_to(i);
	if(t_quit) return 1;
      }
      for(i=1;i<=3-t_skip_cap;i++) {
	threshold_to(i,0,0);
      if(t_quit) return 1;
      }
    }

  // Edit Mode
  else
    {
      if(strcmp(argv[1],"edit") == 0){
	t_data = 1;
	t_mode = 1;
	for(i=1;i<=argc-2;i++){
	  threshold_to(i,argv,1);
	  if(t_quit) return 1;
	}
      }
    }

  // (1) Initialize
  glutInit(&argc, argv);
  glutInitDisplayMode ( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
  glutInitWindowSize (640, 480);
  glutInitWindowPosition (150, 100);
  glutCreateWindow (argv[0]);
  init();

  // (2) Load
  if(t_data) {
    char* argument[]={"\0","0data1.pbm","0data2.pbm","0data3.pbm"};
    if(t_mode) {
      LoadImg((argc-1),argument);
    }
    else LoadImg((4-t_skip_cap),argument);
  }
  else LoadImg(argc,argv);
  if(t_quit) return 1;

  // (3) Draw
  GenMatrix();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glutMainLoop();

  return 0;
  }
