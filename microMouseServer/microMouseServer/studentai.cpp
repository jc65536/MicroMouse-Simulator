
#include "micromouseserver.h"
#include <iostream>
#include <stack>
#include <map>
#include <cstring>
#include <algorithm>

// destination coordinates
const int DX = 11;
const int DY = 7;

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

// a node is any tile with more than 2 exit paths
struct Node {
    int i, x, y;
    std::map<Dir, Node *> adj = {{N, nullptr}, {E, nullptr}, {W, nullptr}, {S, nullptr}};
    Node (int i, int x, int y) : i(i), x(x), y(y) {}
} DEAD_END(-1, -1, -1);

int sq(int x) {
    return x * x;
}

/* Usable functions:
 * bool isWallLeft();
 * bool isWallRight();
 * bool isWallForward();
 * bool moveForward();
 * bool turnLeft();
 * bool turnRight();
 * void foundFinish();
 * void printUI(const char *mesg);
 */

void microMouseServer::studentAI() {
    // do not repeat studentAI()
    static bool firstRun = true;

    // x: mouse current x position; y: mouse current y position; nodeNum: just a counter to identify each node when I print them later
    static int x, y, nodeNum;
    static Dir lastStep;       // always updated to be the same direction as the step we just took

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

    // travel from a node in the specified direction
    // stops at either a node or a dead end, and returns true for stopping at node or false for stopping at dead end
    static auto travel = [&](Dir d) -> bool {
        Dir nextDir = d;
        while (true) {
            step(nextDir);
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
                std::cout << "reached dead end " << x << ", " << y << std::endl;
                return false;
            default:                                            // reached node (crossroads)
                std::cout << "reached node " << x << ", " << y << std::endl;
                return true;
            }
        }
    };

    static Node *map[20][20]; // map[x][y] will tell us if we are at an existing node or an unvisited tile, since map is cleared to 0 by default
    static std::stack<Dir> s;                  // use this stack for backtracking
    static bool goingBack;

    if (firstRun) {
        x = y = nodeNum = 0;
        // we should start at a node, just so that the loop can be simplified
        int startingPaths = test();
        switch (startingPaths) {
        case N:
        case E:
        case W:
        case S:
            travel((Dir) startingPaths);
            break;
        }
        std::memset(map, 0, 400 * sizeof(Node *));
        map[x][y] = new Node(nodeNum++, x, y);  // set the first starting node
        goingBack = firstRun = false;
    }

    Node *currentNode = map[x][y];

    // if we were retreating from a dead end, no need to push an extraneous last step onto the stack - it's already there
    if (!goingBack)
        s.push(opposite(lastStep));
    else
        currentNode->adj[opposite(lastStep)] = &DEAD_END;
    goingBack = false;

    int paths = test();

    // allows the mouse to go in the direction that minimizes its distance from the destination
    std::pair<Dir, int> distanceDifferences[4] = {{N, sq(DX - x) + sq(DY - (y + 1))}, {E, sq(DX - (x + 1)) + sq(DY - y)}, {W, sq(DX - (x - 1)) + sq(DY - y)}, {S, sq(DX - x) + sq(DY - (y - 1))}};
    std::sort(distanceDifferences, distanceDifferences + 4, [&](std::pair<Dir, int> &a, std::pair<Dir, int> &b) -> bool {
        return a.second < b.second;
    });

    for (int i = 0; i < 4; i++) {
        Dir d = distanceDifferences[i].first;
        if (paths & d && currentNode->adj[d] != &DEAD_END && d != s.top()) {            // checks if the direction is open, not already marked as a dead end, and not the direction we just came from
            bool t = travel(d);
            if (t) {
                bool testflag = false;
                if (!map[x][y]) {
                    testflag = true;
                    std::cout << "new node created" << std::endl;
                    map[x][y] = new Node(nodeNum++, x, y);
                }
                map[x][y]->adj[opposite(lastStep)] = currentNode;                       // exchanges info between the two nodes
                currentNode->adj[d] = map[x][y];
                if (testflag)
                    goto end;               // Please forgive my sin, I only wanted to skip the backtracking code
                else
                    travel(opposite(lastStep));
            } else {
                travel(opposite(lastStep));             // if dead end then retreat
                currentNode->adj[d] = &DEAD_END;
            }
        }
    }
    std::cout << "going back" << std::endl;
    travel(s.top());                // backtrack
    s.pop();                        // if no direction yielded a node (i.e. all directions were dead ends), this node is effectively a dead end too
    goingBack = true;

end:
    // prints all the nodes, so you can keep track of the mouse's travel history
    // note that the printout is rotated, so you need to turn your head
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 20; j++) {
            if (map[i][j])
                std::cout << map[i][j]->i << " ";
            else
                std::cout << "  ";
        }
        std::cout << std::endl;
    }

    if (x == DX && y == DY) {
        std::cout << "reached destination " << x << ", " << y << std::endl;
        foundFinish();
        firstRun = true;
    }
}
