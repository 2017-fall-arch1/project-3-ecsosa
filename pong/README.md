


# Project 3: LCD Game

## Libraries

- timerLib: Provides code to configure Timer A to generate watchdog timer interrupts at 250 Hz

- p2SwLib: Provides an interrupt-driven driver for the four switches on the LCD board and a demo program illustrating its intended functionality.

- lcdLib: Provides low-level lcd control primitives, defines several fonts, 
and a simple demo program that uses them.

- shapeLib: Provides an translatable model for shapes that can be translated 
and rendered as layers.

- circleLib: Provides a circle model as a vector of demi-chord lengths,
pre-computed circles as layers with a variety of radii, 
and a demonstration program that renders a circle.


## Pong game

- shape-motion-demo: A demonstration program that uses shapeLib to represent
and render shapes that move.

Firstly I create the shapes I want to use for my pong game: two paddles, a ball and a board.

- The Mov Layer method simliar to the one provided by the professor creates alinked list of layer references. the velocity represents one iteration of changes.

- MovLayerDraw links the movLayer with layer like figures

- The moveBall method has a lot of functions but overall is the one in charge of making the layer ball move with the correct restrictions. If the ball is about to touch topLeft or bottomRight then the player will score a point that will be deisplayed on the screen and alsoit will start a new round where the ball will be placed at the center of the board. There is also a  buzzer that makes sounds when then there is a point scored. Also when the paddles move, it can be heared a buzz.

- moveDown & moveUp are methods that make the paddles move up and down, this is succeded by changing the direction of the paddles parm ml is the moving shape to be advanced and param fence is the region which will serve as boundary for ml.

In the main all methods are implemented, the buzzer is getting called.

in wdt_c_handler is where the ball is being moved.


## How to play:

Click the switches:
s1&s2 belong to player 1 (the one with red paddle)
s3&s4 belong to the player 2 (pink paddle)

by clicking on s1 or s3 the paddles will move up, in the other hand if you click on s2 and s4 paddles will go down.

The score is displayed on the top side of the board. left side belongs to player 1 and right side belongs to player2.

The game is played until one player scores 3 points.

In this project I worked by myself with the code provided by the professor and also I received lots of help and advices from some classmates, Bianca, Laura and Adrian.




