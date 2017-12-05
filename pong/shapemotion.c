/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#include "buzzer.h"
#include "buzzer.c"

#define GREEN_LED BIT6

u_char player1Score = '0';
u_char player2Score = '0';

static int state = 0;


AbRect rect10 = {abRectGetBounds, abRectCheck, {15,2}}; /**< 10x10 rectangle */
//AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 30};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer layer4 = {
  (AbShape *)&rect10,
  {(screenWidth/2), (screenHeight/2)-68}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_PINK,
  0
};
  

//Layer layer3 = {		/**< Layer with an orange circle */
//  (AbShape *)&circle8,
//  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
//  {0,0}, {0,0},				    /* last & next pos */
//  COLOR_VIOLET,
//  &layer4,
//};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &layer4
};

Layer layer1 = {		/**< Layer with a red square */
  (AbShape *)&rect10,
  {screenWidth/2, (screenHeight/2)+67}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &fieldLayer,
};

Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle8,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_ORANGE,
  &layer1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &layer4, {1,1}, 0 }; /**< not all layers move *///pink rect
MovLayer ml1 = { &layer1, {1,2}, 0};//red rectangle 
MovLayer ml0 = { &layer0, {2,1}, 0}; //bola

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

///////////////////////////////////////////////////////////////////////
void moveBall(MovLayer *ml, Region *fence1, MovLayer *ml2, MovLayer *ml3)

{

  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  int velocity;
  
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++){
      if((shapeBoundary.topLeft.axes[axis] < fence1->topLeft.axes[axis]) ||
	 (shapeBoundary.botRight.axes[axis] > fence1->botRight.axes[axis])){
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      else if((abShapeCheck(ml2->layer->abShape, &ml2->layer->posNext, &ml->layer->posNext))){
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      else if((abShapeCheck(ml3->layer->abShape, &ml3->layer->posNext, &ml->layer->posNext))){
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      else if((shapeBoundary.topLeft.axes[0] < fence1->topLeft.axes[0])){
	newPos.axes[0] = screenWidth/2;
	newPos.axes[1] = screenHeight/2;
	player2Score = player2Score - 255;
      }
      
      else if((shapeBoundary.botRight.axes[0] > fence1->botRight.axes[0])){
	newPos.axes[0] = screenWidth/2;
	newPos.axes[1] = screenHeight/2;
	player1Score = player1Score - 255;
      }
      if(player1Score == '9' || player2Score == '9'){
	state = 1;
      }
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void moveDown(MovLayer *ml, Region *fence)
{

  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;

  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 1; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}
	  
void moveUp(MovLayer *ml, Region *fence)
{
  
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;

  for (; ml; ml = ml->next) {
    vec2Sub(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 1; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
    /**bacgroung color Black**/
}//u_int bgColor = COLOR_BLACK;
//int redrawScreen = 1;
//Region fieldFence;



/////////////////////////////////////////////////////////////////////



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
 Vec2 newPos;
u_char axis;
Region shapeBoundary;
for (; ml; ml = ml->next) {
  vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
  abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
  for (axis = 0; axis < 2; axis ++) {
    if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
    }	/**< if outside of fence */
  } /**< for axis */
  ml->layer->posNext = newPos;
} /**< for ml */
}


u_int bgColor = COLOR_BLUE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{

  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */	     
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);

  
  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  drawString5x7(0,0, "score:", COLOR_BLACK, COLOR_BLUE);

   
  
  for(;;) {
    
  u_int switches = p2sw_read(), i;
  char str[5];
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
      for(i=0;i<4;i++){
	//str[i] = buzzer_init();
        str[i] = (switches & (1<<i)) ? '-' : '0'+i;

	  


      }
      //if(str[0]){ 
      if(!(switches & (1<<0))){
	drawString5x7(60,60, "o",COLOR_BLACK, COLOR_BLUE);
	//buzzer_init();
      }
      if (!(switches & (1<<1))){
	drawString5x7(60,60, "a",COLOR_BLACK, COLOR_BLUE);
	//buzzer_init();
      }
      if (!(switches & (1<<2))){
	drawString5x7(60,60, "l",COLOR_BLACK, COLOR_BLUE);
	//buzzer_init();
      }
      if (!(switches & (1<<3))){
	drawString5x7(60,60, "q",COLOR_BLACK, COLOR_BLUE);
      }

      str[4] = 0;
    //buzzer_init();
      drawString5x7(40,0, str,COLOR_BLACK, COLOR_BLUE);

    
      
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);



    movLayerDraw(&ml3, &layer0);

    movLayerDraw(&ml1, &layer0);

    movLayerDraw(&ml0, &layer0);

    //board score and writting

        drawChar5x7(5, 5, player1Score, COLOR_BLUE, COLOR_BLACK);

    drawChar5x7(115, 5, player2Score, COLOR_BLUE, COLOR_BLACK);

    //drawString5x7(5, 150, "Pong<3", COLOR_WHITE, COLOR_BLACK);

    //drawString5x7(38, 5, "<3Points<3", COLOR_WHITE, COLOR_BLACK);
    //
    // or_sr(0x8);


 
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{

  // u_int switches = p2sw_read();
  //static short count = 0;
    //P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  // count ++;

  // if (count == 15) {
  //mlAdvance(&ml0, &fieldFence);
  //if (p2sw_read())
  //  redrawScreen = 1;
  //count = 0;
  //} 
  //P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
  static short count = 0;
  P1OUT |= GREEN_LED;
  count ++;
  u_int switches = p2sw_read();
  if(count == 10){
    switch(state){
    case 0:
      moveBall(&ml0, &fieldFence, &ml1, &ml3);
      break;
    case 1:
      layerDraw(&layer1);
      if(player1Score > player2Score)
	drawString5x7(4,80, "#1 WON #2 keep trying:P", COLOR_WHITE, COLOR_BLACK);
      else if(player1Score < player2Score)
	drawString5x7(4, 50, "#2 WON #1 try again:P", COLOR_WHITE, COLOR_BLACK);
      break;
    }
    if(switches & (1<<3)){
      moveUp(&ml1, &fieldFence);
    }
    if(switches & (1<<2)){
      moveDown(&ml1, &fieldFence);
    }
    if(switches & (1<<1)){
      moveUp(&ml3, &fieldFence);
    }
    if(switches & (1<<0)){
      moveDown(&ml3, &fieldFence);
    }
    redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;

  

  
  //if(switches && (1<<2)){
  // buzzer_init();
  //}
  
  //if(switches && (1<<1)){
  // buzzer_init();
  // }
  
  //if(switches && (1<<0)){
  //buzzer_init();
  //}

}


