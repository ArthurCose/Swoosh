#pragma once
#include <Swoosh\Segue.h>
#include <Swoosh\Ease.h>
#include <Swoosh\Game.h>

#include "..\ResourcePaths.h"
#include "..\TextureLoader.h"

using namespace swoosh;

/*
This segue was written from the 2004 paper titled
 
"Deforming Pages of 3D Electronic Books"
By Lichan Hong, Stuart K. Card, and Jindong (JD) ChenPalo Alto Research Center

Original paper:
https://www.scribd.com/document/260579490/Hong-DeformingPages-algorithm
----

The concept is clever. Make an invisible cone with a large radius of size theta.
The y axis is the contact point of the 3D cone and the 2D surface.
Project the 2D point from paper to the other side of the cone (2D -> 3D).
OVer time, shrink the radius of the cone to 0. The projection will be flat and to the side.
This model is perfect for page turning.

With minor adjustment to the invisible cone's size, shape, and projection point;
we can emulate a more realistic page turning segue.
*/

class PageTurn : public Segue {
private:
  sf::Texture* temp;
  sf::Texture* pattern;
  sf::Shader shader;
  sf::VertexArray paper;

  // More cells means higher quality effect at the cost of more work for cpu and gpu
  // Bigger cell size = less cells fit
  // Smaller cell size = more cells fit
  void triangleStripulate(int screenWidth, int screenHeight, sf::VertexArray& destination, int cellSize) {
    destination.clear();

    int cols = screenWidth / cellSize;
    int rows = screenHeight / cellSize;

    cellSize /= 2;

    // each grid has 2 triangles which have 3 points (1 point = 1 vertex)
    int total = cols * rows * 2 * 3;
    destination = sf::VertexArray(sf::PrimitiveType::Triangles, 0);

    for (int i = 0; i < cols; i++) {
      for (int j = 0; j < rows; j++) {
        sf::Vertex vertex;
        vertex.color = sf::Color::White;

        sf::Vector2f pos[4] = { 
          sf::Vector2f(i*cellSize      , j*cellSize),
          sf::Vector2f(i*cellSize      , (j + 1)*cellSize),
          sf::Vector2f((i + 1)*cellSize, (j + 1)*cellSize),
          sf::Vector2f((i + 1)*cellSize, j*cellSize) 
        };

        // ccw
        int order[6] = { 0, 2, 1, 0, 3, 2 };

        for (auto o : order) {
          vertex.position = pos[o];
          vertex.texCoords = pos[o];
          destination.append(vertex);
        }
      }
    }
  }

public:
  virtual void onDraw(sf::RenderTexture& surface) {
    double elapsed = getElapsed().asMilliseconds();
    double duration = getDuration().asMilliseconds();
    double alpha = ease::linear(elapsed, duration, 1.0);

    float angle1 = ease::radians(90.0f);
    float angle2 = ease::radians(45.0f);
    float angle3 = ease::radians(4.0f);
    float     A1 = -15.0f;
    float     A2 = -1.5f;
    float     A3 = -50.5f;
    float theta1 = 0.05f;
    float theta2 = 0.5f;
    float theta3 = 10.0f;
    float theta4 = 2.0f;

    float f1, f2, dt;
    double theta = 90.f;
    double A = 0.0f;

    double rho = alpha * ease::pi;

    if (alpha <= 0.15)
    {
      // Start off with a flat page with no deformation at the beginning of a page turn, then begin to curl the page gradually
      // as the hand lifts it off the surface of the book.
      dt = alpha / 0.15;
      f1 = sin(ease::pi * pow(dt, theta1) / 2.0);
      f2 = sin(ease::pi * pow(dt, theta2) / 2.0);
      theta = ease::interpolate(f1, angle1, angle2);
      A = ease::interpolate(f2, A1, A2);
    }
    else if (alpha <= 0.84)
    {
      // Produce the most pronounced curling near the middle of the turn. Here small values of theta and A
      // result in a short, fat cone that distinctly show the curl effect.
      dt = (alpha - 0.15) / 0.25;
      theta = ease::interpolate(dt, angle2, angle3);
      A = ease::interpolate(dt, A2, A3);
    }
    else if (alpha <= 1.0)
    {
      // Near the middle of the turn, the hand has released the page so it can return to its normal form.
      // Ease out the curl until it returns to a flat page at the completion of the turn. More advanced simulations
      // could apply a slight wobble to the page as it falls down like in real life.
      dt = (alpha - 0.4) / 0.6;
      f1 = sin(ease::pi * pow(dt, theta3) / 2.0);
      f2 = sin(ease::pi * pow(dt, theta4) / 2.0);
      theta = ease::interpolate(f1, angle3, angle1);
      A = ease::interpolate(f2, A3, A1);
    }

    this->drawLastActivity(surface);

    surface.display(); // flip and ready the buffer

    if (temp) delete temp;
    temp = new sf::Texture(surface.getTexture()); // Make a copy of the source texture

    shader.setUniform("A", (float)A);
    shader.setUniform("theta", (float)theta);
    shader.setUniform("rho", (float)rho);
    shader.setUniform("texture", *temp);

    sf::RenderStates states;
    states.shader = &shader;

    surface.clear(sf::Color::Transparent);
    surface.draw(paper, states);
    
    surface.display();

    sf::Texture* temp2 = new sf::Texture(surface.getTexture()); // Make a copy of the source texture
    sf::Sprite left(*temp2);

    surface.clear(sf::Color::Transparent);

    this->drawNextActivity(surface);

    surface.display(); // flip and ready the buffer
    sf::Sprite right(surface.getTexture());

    getController().getWindow().draw(right);
    getController().getWindow().draw(left);

    delete temp2;
    surface.clear(sf::Color::Transparent);
  }

  PageTurn(sf::Time duration, Activity* last, Activity* next) : Segue(duration, last, next) {
    /* ... */
    temp = nullptr;
    shader.loadFromFile(TURN_PAGE_VERT_PATH, TURN_PAGE_FRAG_PATH);
    auto size = getController().getWindow().getView().getSize();
    triangleStripulate(size.x, size.y, paper, 10);
  }

  virtual ~PageTurn() { ; }
};
