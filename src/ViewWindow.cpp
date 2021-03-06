#include "ViewWindow.h"

ViewWindow::ViewWindow(const int width, const int height, const char* title) 
    : window(sf::VideoMode(width, height), title, sf::Style::Resize | sf::Style::Titlebar), 
    mouseButtonIsDown(false), 
    rightVertexArray(sf::Lines), 
    leftVertexArray(sf::Lines), 
    zoom(6), eyeDistance(0.1), 
    eyeTarget(7), 
    rotationDensity(100), 
    rotationResistance(0.995), 
    translationMatrix(IdentityMatrix()), 
    rotationMatrix(IdentityMatrix()), 
    mainMatrix(IdentityMatrix()),
    multipliers(),
    rotationSpeedX(0), 
    rotationSpeedY(0),
    offsetX(0), 
    offsetY(0)
{ 
    window.setActive(); 
    int x = sf::VideoMode::getDesktopMode().width;
    int y = sf::VideoMode::getDesktopMode().height;
    window.setPosition(sf::Vector2i(x- width, 0));
};

ViewWindow::~ViewWindow()
{
    window.close();
}

bool ViewWindow::processMessages()
{
    if (window.isOpen())
    {
        HandleEvents();
        heartBeat();
        paint();
        return true;
    }
    else return false;
}
        
void ViewWindow::HandleEvents()
{
    sf::Event event;
    while (window.pollEvent(event))
    {
        closeEvent(event);
        mouseDownEvent(event);
        mouseUpEvent(event);
        mouseMoveEvent(event);
        mouseScrollEvent(event);
        keyEvent(event);
        //obsługa reszty komunikatów
    }
}

//przykładowy komunikat
bool ViewWindow::closeEvent(sf::Event& event)
{
    if (event.type == sf::Event::Closed)
    {
        window.close();
        return true;
    }
    return false;
}

bool ViewWindow::mouseDownEvent(sf::Event& event)
{
    if (event.type == sf::Event::MouseButtonPressed)
    {
        rotationSpeedX = event.mouseButton.x - mousePositionX;
        rotationSpeedY = event.mouseButton.y - mousePositionY;
        mousePositionX = event.mouseButton.x;
        mousePositionY = event.mouseButton.y;
        mouseButtonIsDown = true;
        return true;
    }
    return false;
}

bool ViewWindow::mouseUpEvent(sf::Event& event)
{
    if (event.type == sf::Event::MouseButtonReleased)
    {
        mouseButtonIsDown = false;
        return true;
    }
    return false;
}

bool ViewWindow::keyEvent(sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        switch (event.key.code)
        {
            case sf::Keyboard::Up : offsetY += 0.05*zoom; break;
            case sf::Keyboard::Down : offsetY -= 0.05*zoom; break;
            case sf::Keyboard::Left : offsetX += 0.05*zoom; break;
            case sf::Keyboard::Right : offsetX -= 0.05*zoom; break;
        }
    }
    return false;
}

double sign(double x)
{
    if (x < 0.)
        return -1.;
    return 1.;
}

bool ViewWindow::mouseMoveEvent(sf::Event& event)
{
    if (event.type == sf::Event::MouseMoved)
    {
        if (mouseButtonIsDown)
        {
            rotationSpeedX = rotationSpeedX * 0.1 + (event.mouseMove.x - mousePositionX) * 0.9;
            rotationSpeedY = rotationSpeedY * 0.1 + (event.mouseMove.y - mousePositionY) * 0.9;
        }

        mousePositionX = event.mouseMove.x;
        mousePositionY = event.mouseMove.y;
        return true;
    }
    return false;
}

bool ViewWindow::mouseScrollEvent(sf::Event& event)
{
    if (event.type == sf::Event::MouseWheelScrolled)
    {
        zoom *= pow(0.9, event.mouseWheelScroll.delta);
        return true;
    }
    return false;
}

void ViewWindow::UpdateEyeMatrixes()
{
    double a = 1.570796326794897-atan2(eyeTarget, eyeDistance/2);
    multipliers.leftMatrix = CreateMoveMatrix(eyeDistance/2, 0, 0) * CreateRotationMatrix(a, 1) * mainMatrix;
    multipliers.rightMatrix = CreateMoveMatrix(-eyeDistance/2, 0, 0) * CreateRotationMatrix(-a, 1) * mainMatrix;
}

void ViewWindow::paint()
{
    RenderTo(window);    
    window.display();
}

void ViewWindow::Render()
{
    UpdateEyeMatrixes();
    multipliers.calculate();
}

void ViewWindow::Update(const Matrix4& wxTranslation, const Matrix4& wxRotation) 
{
    rotationSpeedX = rotationSpeedY = 0.;
    translationMatrix = wxTranslation;
    rotationMatrix = wxRotation;
    Render();
}

void ViewWindow::setData(const std::vector<Section>& newData)
{
    multipliers.vertexArrayLeft = &leftVertexArray;
    multipliers.vertexArrayRight = &rightVertexArray;
    multipliers.sections = &newData;
        
    translationMatrix = IdentityMatrix(); 
    rotationMatrix = IdentityMatrix(); 
    mainMatrix = IdentityMatrix();
    rotationSpeedX = 0;
    rotationSpeedY = 0;
    offsetX = 0;
    offsetY = 0;
    
    UpdateEyeMatrixes();
    multipliers.asyncCalculate();
    
    Point min, max;
    for (const auto& s : newData)
    {
        max.x = std::max(std::max(s.begin.x, s.end.x), center.x);
        max.y = std::max(std::max(s.begin.y, s.end.y), center.y);
        max.z = std::max(std::max(s.begin.z, s.end.z), center.z);
        min.x = std::min(std::min(s.begin.x, s.end.x), center.x);
        min.y = std::min(std::min(s.begin.y, s.end.y), center.y);
        min.z = std::min(std::min(s.begin.z, s.end.z), center.z);
    }
    center = (min+max)*0.5;
    
    multipliers.wait();
}

void ViewWindow::setEyeFocus(double focus)
{
    eyeTarget = focus;
    Render();
}

void ViewWindow::setEyeDistance(double distance)
{
    eyeDistance = distance;
    Render();
}

void ViewWindow::heartBeat()
{
    rotationMatrix = CreateRotationMatrix(-rotationSpeedX/rotationDensity, 1) * CreateRotationMatrix(rotationSpeedY/rotationDensity, 0) * rotationMatrix;
    mainMatrix = translationMatrix * CreateMoveMatrix(center.x, center.y, center.z) * rotationMatrix * CreateMoveMatrix(-center.x, -center.y, -center.z);
 
    rotationSpeedX *= rotationResistance;
    rotationSpeedY *= rotationResistance;
    
    if (fabs(rotationSpeedX) < 0.01)
        rotationSpeedX = 0;
    if (fabs(rotationSpeedY) < 0.01)
        rotationSpeedY = 0;
    
    if (rotationSpeedX != 0 || rotationSpeedY != 0)    
        Render();
}

void ViewWindow::SaveToFile(const std::string& fileName, const unsigned int width, const unsigned int height, const bool windowProportion) const
{
    sf::RenderTexture renderTexture;
    renderTexture.create(width, height);

    sf::RenderStates state = sf::RenderStates::Default;
    state.transform.scale(double(width) / window.getSize().x, double(height) / window.getSize().y);
    RenderTo(renderTexture, windowProportion);
    sf::Image image = renderTexture.getTexture().copyToImage();
    image.flipVertically();
    image.saveToFile(fileName);
}

void ViewWindow::setColors(int r1, int g1, int b1, int r2, int g2, int b2)
{
    multipliers.setColors(sf::Color(r1, g1, b1), sf::Color(r2, g2, b2));

    Render();
}

sf::Vector2u ViewWindow::getSize() const
{
    return window.getSize();
}

void ViewWindow::RenderTo(sf::RenderTarget& target, const bool windowProportion) const
{
    target.clear(sf::Color::Black);
    auto size = windowProportion?window.getSize():target.getSize();
    double k = (double)size.x/size.y;

    target.setView(sf::View(sf::FloatRect(offsetX-zoom/2*k, offsetY-zoom/2, zoom*k, zoom)));
    
    auto state = sf::RenderStates::Default;
    state.blendMode = sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add);

    target.draw(leftVertexArray, state);
    target.draw(rightVertexArray, state);
}
