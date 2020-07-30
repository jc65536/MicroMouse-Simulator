#include "micromouseserver.h"
#include <iostream>
#include <stack>
#include <map>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <queue>

// cardinal directions
enum Dir { N = 0b0001, E = 0b0010, W = 0b0100, S = 0b1000 };

Dir opposite(Dir d) {
    switch (d) {
    case N:
        return S;
    case E:
        return W;
    case W:
        return E;
    case S:
        return N;
    }
}

struct Node;

// I didn't want to use std::pair because pair.first isn't as intuitive as Pair.node
struct Pair {
    Node *node;
    int d;
};

// a node is any tile with more than 2 exit paths, with the exception of the root node (origin) and destination node
struct Node {
    int i, x, y;
    std::map<Dir, Pair> adj;                // maps the 4 directions to either a node and the distance to that node, or &DEAD_END
    int d = INT_MAX;                        // shortest distance from origin
    Node *prev;                             // the previous node that minimizes its distance from the origin - these pointers form the optimal node chain
    Node (int i, int x, int y) : i(i), x(x), y(y) {}
};

void microMouseServer::studentAI() {
    // origin coordinates
    static int OX = 0;
    static int OY = 0;

    // destination coordinates
    static int DX = 11;
    static int DY = 7;

    static bool newRun = true;              // state boolean that resets every time you hit "Start Run"
    static bool firstRun = true;            // resets every time you change map - indicate map change by enclosing the starting tile in walls and hitting "Start Run"
    static bool graphBuilding = true;       // if the mouse is currently exploring all nodes to build the node graph

    static int x, y, nodeNum;               // x, y are updated every step; nodeNum is just a counter to identify each node when I print them later
    static Dir lastStep;                    // always updated to be the same direction as the step we just took

    static Node DEAD_END(-1, -1, -1);
    static Node *map[20][20];               // map[x][y] will give us a pointer to an existing node or a false pointer
    static std::stack<Dir> s;               // use this stack for backtracking
    static Node *rootNode;
    static std::stack<Dir> optimalPath;
    static std::stack<Dir> pathCopy;

    // sets a bit for open directions and clears a bit for blocked directions
    // thus, you can & the return with any Dir and see if that direction is open
    static auto test = [&]() -> int {
        int r = isWallForward() + isWallRight() * 2 + isWallLeft() * 4;
        turnRight();
        r += isWallRight() * 8;
        turnLeft();
        r ^= 0b1111;
        return r;
    };

    // take a step in the specified cardinal direction
    static auto step = [&](Dir d) {
        lastStep = d;
        switch (d) {
        case N:
            y += moveForward();
            break;
        case E:
            turnRight();
            x += moveForward();
            turnLeft();
            break;
        case W:
            turnLeft();
            x -= moveForward();
            turnRight();
            break;
        case S:
            turnRight();
            turnRight();
            y -= moveForward();
            turnRight();
            turnRight();
        }
    };

    // travel from a node in the specified direction and stops at either a node or a dead end
    // returns the number of steps taken to reach that node (or 0 for dead end)
    static auto travel = [&](Dir d) -> int {
        Dir nextDir = d;
        int steps = 0;
        while (true) {
            step(nextDir);
            steps++;
            if (map[x][y] == rootNode || (x == DX && y == DY)) {               // even though rootNode/destNode might not have 3 open paths, it's still a node
                return steps;
            }
            int paths = test();
            paths &= opposite(lastStep) ^ 0b1111;               // mask out the direction we came from
            switch (paths) {
            case N:                                             // continue travelling
            case E:                                             //
            case W:                                             //
            case S:                                             //
                nextDir = (Dir) paths;
                break;
            case 0:                                             // dead end
                return 0;
            default:                                            // reached node (crossroads)
                return steps;
            }
        }
    };

    if (newRun) {
        x = OX;
        y = OY;
        if (test() == 0) {              // resets firstRun if trapped in a 1x1 cell
            printUI("Map reset.");
            firstRun = true;
            graphBuilding = true;
            foundFinish();
            return;
        }
        if (firstRun) {
            nodeNum = 0;
            memset(map, 0, 400 * sizeof(Node *));
            map[OX][OY] = new Node(nodeNum++, OX, OY);
            rootNode = map[OX][OY];
            rootNode->d = 0;
            optimalPath = std::stack<Dir>();                // clear optimalPath
        }
        pathCopy = optimalPath;
        newRun = false;
    }

    if (firstRun) {
        if (graphBuilding) {
            // explore all nodes
            Node *currentNode = map[x][y];
            int paths = test();
            for (int i = 0; i < 4; i++) {
                Dir d = Dir(1 << i);
                if (paths & d) {
                    if (!currentNode->adj[d].node) {
                        int t = travel(d);              // t contains the distance we just travelled
                        if (t) {
                            if (map[x][y]) {
                                // exchanges info between the two nodes
                                map[x][y]->adj[opposite(lastStep)] = {currentNode, t};
                                currentNode->adj[d] = {map[x][y], t};
                                travel(opposite(lastStep));
                            } else {
                                map[x][y] = new Node(nodeNum++, x, y);
                                map[x][y]->adj[opposite(lastStep)] = {currentNode, t};
                                currentNode->adj[d] = {map[x][y], t};
                                // push opposite of last step so we can backtrack later
                                s.push(opposite(lastStep));
                                // skip the rest of the code and iterate again for the new node
                                return;
                            }
                        } else {
                            // retreat since we hit a dead end
                            travel(opposite(lastStep));
                            currentNode->adj[d] = {&DEAD_END, 0};
                        }
                    }
                } else {
                    currentNode->adj[d] = {&DEAD_END, 0};
                }
            }

            if (currentNode == rootNode) {
                // we are done building the node graph if we had to backtrack all the way to rootNode
                graphBuilding = false;
            } else {
                // backtrack
                travel(s.top());
                s.pop();
                // a node with 3 dead ends is effectively a dead end itself
                int deadEnds = 0;
                for (int i = 0; i < 4; i++) {
                    deadEnds += currentNode->adj[Dir(1 << i)].node == &DEAD_END;
                }
                // the destination tile is always a node, even if it has 3 dead ends
                if (deadEnds == 3 && currentNode != map[DX][DY]) {
                    map[currentNode->x][currentNode->y] = &DEAD_END;
                    map[x][y]->adj[opposite(lastStep)] = {&DEAD_END, 0};
                }
            }
        } else {
            // prints node map
            std::cout << std::endl;
            for (int i = 0; i < 20; i++) {
                for (int j = 0; j < 20; j++) {
                    if (map[j][19 - i] && map[j][19 - i] != &DEAD_END)
                        std::cout << std::setw(3) << map[j][19 - i]->i;
                    else
                        std::cout << "  .";
                }
                std::cout << std::endl;
            }

            // calculate optimal path
            std::queue<Node *> q;               // a node queue for our breadth-first-search approach to traversing the graph
            q.push(rootNode);
            bool visited[20][20];               // visited[x][y] tells us if we have already calculated the node at x, y
            memset(visited, false, 400 * sizeof(bool));
            visited[rootNode->x][rootNode->y] = true;

            // traverses the node graph breadth-first (i.e. look at all neighbors first before moving on to neighbors' neighbors)
            while (q.size() > 0) {
                Node *currentNode = q.front();
                for (int i = 0; i < 4; i++) {
                    Pair p = currentNode->adj[Dir(1 << i)];
                    if (p.node != &DEAD_END) {
                        // updates best distance and prev node if the path from currentNode is shorter
                        if (currentNode->d + p.d < p.node->d) {
                            p.node->d = currentNode->d + p.d;
                            p.node->prev = currentNode;
                            q.push(p.node);             // fixes the problem of not updating nodes when a better path is found, because the algo will check any updated nodes again
                        }
                        if (!visited[p.node->x][p.node->y]) {
                            q.push(p.node);
                            visited[p.node->x][p.node->y] = true;
                        }
                    }
                }
                q.pop();
            }

            Node *n = map[DX][DY];
            // follows prev node chain from destination back to origin, and builds a direction stack
            std::cout << std::endl << "Optimal path (total " << n->d << "):" << std::endl;
            while (n != rootNode) {
                std::cout << n->i << "<-" << n->prev->i << " (" << (n->d - n->prev->d) << ")" << std::endl;
                int min = INT_MAX;
                Dir d = N;
                for (int i = 0; i < 4; i++) {
                    Pair p = n->prev->adj[Dir(1 << i)];
                    if (p.node == n && p.d < min) {
                        min = p.d;
                        d = Dir(1 << i);
                    }
                }
                optimalPath.push(d);
                n = n->prev;
            }
            printUI("Optimal path calculated (see stdout for more).");
            pathCopy = optimalPath;             // need a copy since we don't want to lose optimalPath
            firstRun = false;                   // when this is false, every time studentAI() is called we will skip to the else statement below
        }
    } else {
        // follow optimal path
        travel(pathCopy.top());
        pathCopy.pop();

        if (pathCopy.empty()) {
            foundFinish();
            newRun = true;
        }
    }

}
