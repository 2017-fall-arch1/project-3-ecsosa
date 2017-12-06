
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

u_char score1 = '0';//players score
u_char score2 = '0';

static int state = 0;

AbRect rect10 = {abRectGetBounds, abRectCheck, {2,15}}; /**<10x10 rectangle */
AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  0
};

Layer layer4 = {
  (AbShape *)&rect10,
  {(screenWidth/2)+40, (screenHeight/2)}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_PINK,
  &fieldLayer
};

Layer layer1 = {		/**< Layer with a red square */
  (AbShape *)&rect10,
  {(screenWidth/2)-40, (screenHeight/2)}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &layer4,
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
MovLayer ml3 = { &layer4, {0,3}, 0 }; /**< not all layers move *///pink rect
MovLayer ml1 = { &layer1, {0,3}, 0};//red rectangle 
MovLayer ml0 = { &layer0, {1,2}, 0}; //bolax

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

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 *  ml2 and ml3 are the paddles
 */

void moveBall(MovLayer *ml, Region *fence1, MovLayer *ml2, MovLayer *ml3)

{

  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  int velocity;
  
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2 ; axis ++){
      if((shapeBoundary.topLeft.axes[axis] < fence1->topLeft.axes[axis]) ||
	 (shapeBoundary.botRight.axes[axis] > fence1->botRight.axes[axis])){
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      else if((abShapeCheck(ml2->layer->abShape, &ml2->layer->posNext, &ml->layer->posNext))){//square
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      else if((abShapeCheck(ml3->layer->abShape, &ml3->layer->posNext, &ml->layer->posNext))){//square
	velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      else if((shapeBoundary.topLeft.axes[0] < fence1->topLeft.axes[0])){//check bounds
	newPos.axes[0] = screenWidth/2;//restart in this position
	newPos.axes[1] = screenHeight/2;
	score2 = score2 - 255;//new score
	buzzer_init(400);
      }
      
      else if((shapeBoundary.botRight.axes[0] > fence1->botRight.axes[0])){//check bounds
	newPos.axes[0] = screenWidth/2;//restart in new position
	newPos.axes[1] = screenHeight/2;
	score1 = score1 - 255; //new score
	buzzer_init(400);
      }
      else
	buzzer_init(0);
      if(score1 == '3' || score2 == '3'){//checking score
	state = 1;//if score new state
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
    
  u_int switches = p2sw_read();
  char str[5];
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    if(!(switches & (1<<0))){
    	buzzer_init(100);
    }
    if (!(switches & (1<<1))){
    	buzzer_init(100);
    }
    if (!(switches & (1<<2))){
      buzzer_init(100);
    }
    if (!(switches & (1<<3))){
      buzzer_init(100);
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
    movLayerDraw(&ml3, &layer0);
    movLayerDraw(&ml1, &layer0);

    //board score and writting

    drawChar5x7(50, 0, score1 , COLOR_BLACK, COLOR_BLUE);
    drawString5x7(60, 0, ":" , COLOR_BLACK, COLOR_BLUE);
    drawChar5x7(70, 0, score2, COLOR_BLACK, COLOR_BLUE);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;
  count ++;
  u_int switches = p2sw_read();
  if(count == 15){
    switch(state){
    case 0:
      moveBall(&ml0, &fieldFence, &ml1, &ml3);
      break;
    case 1:
      layerDraw(&layer0);
      if(score1 > score2)
	drawString5x7(25,20, "player 1 won!!", COLOR_WHITE, COLOR_BLUE);
      else if(score1 < score2)
	drawString5x7(25, 60, "player 2 won!!", COLOR_WHITE, COLOR_BLUE);
      break;
    }
    if(switches & (1<<3)){
      moveUp(&ml3, &fieldFence);
    }
    if(switches & (1<<2)){
      moveDown(&ml3, &fieldFence);
    }
    if(switches & (1<<1)){
      moveUp(&ml1, &fieldFence);
    }
    if(switches & (1<<0)){
      moveDown(&ml1, &fieldFence);
    }
    redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;
}


