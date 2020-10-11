#pragma once
#include <Swoosh/EmbedGLSL.h>
#include <Swoosh/Segue.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Shaders.h>

using namespace swoosh;

/**
  @class BlurFadeIn
  @brief Blurs both screens and fades into the next while the last fades out
  @warning COSTLY! For mobile, blurring will be turned off and it will be a simple fade
*/
class BlurFadeIn : public Segue {
private:
  glsl::FastGaussianBlur shader;
  sf::Texture last, next;
  bool firstPass{ true };
public:
  void onDraw(sf::RenderTexture& surface) override {
    double elapsed = getElapsed().asMilliseconds();
    double duration = getDuration().asMilliseconds();
    double alpha = ease::wideParabola(elapsed, duration, 1.0);
    const bool optimized = getController().isOptimizedForPerformance();

    shader.setPower((float)alpha * 8.f);

    surface.clear(this->getLastActivityBGColor());
    sf::Texture temp, temp2;

    if (firstPass || !optimized) {
      this->drawLastActivity(surface);

      surface.display(); // flip and ready the buffer
      last = temp = sf::Texture(surface.getTexture()); // Make a copy of the source texture
    }
    else {
      temp = last;
    }

    shader.setTexture(&temp);
    shader.apply(surface);

    surface.display();
    temp = sf::Texture(surface.getTexture());

    surface.clear(this->getNextActivityBGColor());

    if (firstPass || !optimized) {
      this->drawNextActivity(surface);

      surface.display(); // flip and ready the buffer
      next = temp2 = sf::Texture(surface.getTexture()); // Make a copy of the source texture
    }
    else {
      temp2 = next;
    }

    shader.setTexture(&temp2);
    shader.apply(surface);

    surface.display();
    temp2 = sf::Texture(surface.getTexture());

    sf::Sprite sprite, sprite2;
    sprite.setTexture(temp);
    sprite2.setTexture(temp2);

    surface.clear(sf::Color::Transparent);
    alpha = ease::linear(elapsed, duration, 1.0);

    sprite.setColor(sf::Color(255, 255, 255, (sf::Uint8)(255.0 * (1-alpha))));
    sprite2.setColor(sf::Color(255, 255, 255, (sf::Uint8)(255.0 * alpha)));

    surface.draw(sprite);
    surface.draw(sprite2);

    firstPass = false;
  }

  BlurFadeIn(sf::Time duration, Activity* last, Activity* next) 
    // use 60 kernels in regular performance and 10 for optimized...
    : Segue(duration, last, next), shader(next->getController().isOptimizedForPerformance()? 10 : 60) {
    /*...*/
  }

  ~BlurFadeIn() { ; }
};
