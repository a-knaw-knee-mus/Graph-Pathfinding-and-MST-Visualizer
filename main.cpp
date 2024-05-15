#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <cmath>
#include <iostream>
#include <algorithm>
using namespace sf;
using namespace std;

// get the line shapes that represent connections between nodes to be drawn to the window
vector<VertexArray> getConnectionsBetweenNodes(vector<vector<shared_ptr<CircleShape>>> connectionData) {
    vector<VertexArray> connections;

    for (auto& c: connectionData) {
        VertexArray line(Lines, 2);
        line[0].position = c[0]->getPosition();
        line[0].color = Color::Red;
        line[1].position = c[1]->getPosition();
        line[1].color = Color::Blue;
        connections.push_back(line);
    }
    return connections;
}

// check if connection between 2 nodes exists in either direction
bool doesConnectionExist(vector<vector<shared_ptr<CircleShape>>> connectionData, shared_ptr<CircleShape> start, shared_ptr<CircleShape> end) {
    vector<shared_ptr<CircleShape>> v = {start, end};
    if (find(connectionData.begin(), connectionData.end(), v) != connectionData.end()) {
        return true;
    }

    v = {end, start};
    if (find(connectionData.begin(), connectionData.end(), v) != connectionData.end()) {
        return true;
    }

    return false;
}

int main() {
    RenderWindow window(VideoMode(600, 600), "Graph Pathfinding");

    // Define two circles
    const int circleRadius = 30;
    vector<shared_ptr<CircleShape>> nodes;
    vector<vector<shared_ptr<CircleShape>>> connectionData;

    CircleShape node1(circleRadius-4);
    node1.setPosition(100, 100);
    node1.setOutlineColor(Color::Black);
    node1.setOutlineThickness(2);
    node1.setOrigin({ node1.getRadius(), node1.getRadius() });
    nodes.push_back(make_shared<CircleShape>(node1));

    CircleShape node2(circleRadius-4);
    node2.setPosition(300, 300);
    node2.setOutlineColor(Color::Black);
    node2.setOutlineThickness(2);
    node2.setOrigin({ node2.getRadius(), node2.getRadius() });
    nodes.push_back(make_shared<CircleShape>(node2));

    connectionData.push_back({nodes[0], nodes[1]});

    int currCircle = -1;
    Vector2f offset; // Offset for dragging
    int lineStartIdx=-1;
    bool isShiftPressed = false;

    while (window.isOpen()) {
        Event event{};
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            // delete something
            else if (event.type == Event::KeyPressed && event.key.code == Keyboard::LShift) {
                isShiftPressed = true;
            }
            else if (event.type == Event::KeyReleased && event.key.code == Keyboard::LShift) {
                isShiftPressed = false;
            }
            // delete node
            else if (isShiftPressed && Mouse::isButtonPressed(Mouse::Left)) {
                Vector2f mousePos = Vector2f(Mouse::getPosition(window));
                if (!(mousePos.x >= 0 && mousePos.x <= window.getSize().x && mousePos.y >= 0 && mousePos.y <= window.getSize().y)) continue;
                for (int i = 0; i < nodes.size(); i++) {
                    if (nodes[i]->getGlobalBounds().contains(mousePos)) {
                        shared_ptr<CircleShape> removedNode = nodes[i];
                        nodes.erase(nodes.begin()+i);
                        for (auto it = connectionData.begin(); it != connectionData.end();) {
                            bool deleteConnection = false;
                            for (const auto& node : *it) {
                                if (node == removedNode) { // Compare with the iterator, not nodes[0]
                                    deleteConnection = true;
                                    break;
                                }
                            }
                            if (deleteConnection) {
                                it = connectionData.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                }
            }

            // add connection
            else if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Right) {
                Vector2f mousePos = Vector2f(Mouse::getPosition(window));
                if (!(mousePos.x >= 0 && mousePos.x <= window.getSize().x && mousePos.y >= 0 && mousePos.y <= window.getSize().y)) continue;
                for (int i = 0; i < nodes.size(); i++) {
                    if (nodes[i]->getGlobalBounds().contains(mousePos)) {
                        lineStartIdx = i;
                        break;
                    }
                }
            }
            else if (lineStartIdx!=-1 && event.type == Event::MouseButtonReleased && event.mouseButton.button == Mouse::Right) {
                Vector2f mousePos = Vector2f(Mouse::getPosition(window));
                if (!(mousePos.x >= 0 && mousePos.x <= window.getSize().x && mousePos.y >= 0 && mousePos.y <= window.getSize().y)) continue;
                for (int lineEndIdx = 0; lineEndIdx < nodes.size(); lineEndIdx++) {
                    if (nodes[lineEndIdx]->getGlobalBounds().contains(mousePos)) {
                        if (!doesConnectionExist(connectionData, nodes[lineStartIdx], nodes[lineEndIdx])) {
                            connectionData.push_back({nodes[lineStartIdx], nodes[lineEndIdx]});
                        }

                        lineStartIdx = -1;
                        break;
                    }
                }
            }

            // get current held node
            else if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                Vector2f mousePos = Vector2f(Mouse::getPosition(window));
                if (!(mousePos.x >= 0 && mousePos.x <= window.getSize().x && mousePos.y >= 0 && mousePos.y <= window.getSize().y)) continue;
                for (int i = 0; i < nodes.size(); i++) {
                    if (nodes[i]->getGlobalBounds().contains(mousePos)) {
                        offset = mousePos - nodes[i]->getPosition();
                        currCircle = i;
                    }
                }
            }
            else if (currCircle != -1 && event.type == Event::MouseButtonReleased) {
                currCircle = -1;
            }

            // add new node
            else if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::A) {
                    // Generate a new circle
                    CircleShape newCircle(circleRadius-4);
                    Vector2f newPos;
                    bool positionFound = false;
                    for (float y = circleRadius+10; y < window.getSize().y-circleRadius-10; y += circleRadius * 2) {
                        for (float x = circleRadius+10; x < window.getSize().x-circleRadius-10; x += circleRadius * 2) {
                            newPos = Vector2f(x, y);
                            bool validPosition = true;
                            for (const auto& existingNode : nodes) {
                                float combinedRadius = newCircle.getRadius() + existingNode->getRadius();
                                Vector2f centerDiff = newPos - existingNode->getPosition();
                                float centerDist = sqrt(centerDiff.x * centerDiff.x + centerDiff.y * centerDiff.y);
                                if (centerDist < (combinedRadius + (circleRadius*0.7))) { // Ensure new circle doesn't overlap or come too close
                                    validPosition = false;
                                    break;
                                }
                            }
                            if (validPosition) {
                                positionFound = true;
                                break;
                            }
                        }
                        if (positionFound) break;
                    }
                    if (positionFound) {
                        newCircle.setPosition(newPos);
                        newCircle.setOutlineColor(Color::Black);
                        newCircle.setOutlineThickness(2);
                        newCircle.setOrigin({ newCircle.getRadius(), newCircle.getRadius() });
                        nodes.push_back(make_shared<CircleShape>(newCircle));
                    } else {
                        cout << "No possible spot found" << endl;
                    }
                }
            }
        }

        // Dragging logic
        if (currCircle != -1 && Mouse::isButtonPressed(Mouse::Left)) {
            Vector2f mousePos = Vector2f(Mouse::getPosition(window));

            Vector2f newPos = mousePos - offset; // Calculate potential new position

            // Check collision before updating position
            bool collisionDetected = false;
            for (int i = 0; i < nodes.size(); i++) {
                if (i == currCircle) continue;
                Vector2f currCirclePoints = newPos;
                Vector2f otherCirclePoints = nodes[i]->getPosition();
                float distance = sqrt(pow(currCirclePoints.x - otherCirclePoints.x, 2) + pow(currCirclePoints.y - otherCirclePoints.y, 2));
                if (distance < nodes[currCircle]->getRadius() + nodes[i]->getRadius() + (circleRadius*0.7)) {
                    collisionDetected = true;
                    break;
                }
            }

            if (!collisionDetected) {
                nodes[currCircle]->setPosition(newPos); // Update position if no collision detected
            }
        }

        window.clear(Color::White);
        for (const auto& connection: getConnectionsBetweenNodes(connectionData)) {
            window.draw(connection);
        }
        // cout << nodes[0].getPosition().x << " " << nodes[0].getPosition().y << endl;
        for (auto& circle : nodes) {
            window.draw(*circle);
        }
        window.display();
    }

    return 0;
}
