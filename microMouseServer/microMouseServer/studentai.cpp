
#include "micromouseserver.h"
#include <iostream>
#include <stack>
#include <map>
#include <cstring>

// destination coordinates
const int DX = 10;
const int DY = 6;

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
    if (!firstRun) return;
    firstRun = false;

    // x: mouse current x position; y: mouse current y position; nodeNum: just a counter to identify each node when I print them later
    int x = 0, y = 0, nodeNum = 0;
    Dir lastStep;       // always updated to be the same direction as the step we just took

    // sets a bit for open directions and clears a bit for blocked directions
    // thus, you can & the return with any Dir and see if that direction is open
    auto test = [&]() -> int {
        int r = isWallForward() + isWallRight() * 2 + isWallLeft() * 4;
        turnRight();
        r += isWallRight() * 8;
        turnLeft();
        r ^= 0b1111;
        return r;
    };

    // take a step in the specified cardinal direction
    auto step = [&](Dir d) {
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
    auto travel = [&](Dir d) -> bool {
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

    Node *map[20][20];                  // map[x][y] will tell us if we are at an existing node or an unvisited tile, since map is cleared to 0 by default
    std::memset(map, 0, 400 * sizeof(Node *));
    std::stack<Dir> s;                  // use this stack for backtracking
    map[x][y] = new Node(nodeNum++, x, y);  // set the first starting node

    bool goingBack = false;
    while (x != DX && y != DY) {
        Node *currentNode = map[x][y];

        // if we were retreating from a dead end, no need to push an extraneous last step onto the stack - it's already there
        if (!goingBack)
            s.push(opposite(lastStep));
        else
            currentNode->adj[opposite(lastStep)] = &DEAD_END;
        goingBack = false;

        int paths = test();
        for (int i = 0; i < 4; i++) {
            Dir d = Dir(1 << i);
            if (paths & d && currentNode->adj[d] != &DEAD_END && d != s.top()) {            // checks if the direction is open, not already marked as a dead end, and not the direction we just came from
                bool t = travel(d);
                if (t) {
                    if (!map[x][y]) {
                        std::cout << "new node created" << std::endl;
                        map[x][y] = new Node(nodeNum++, x, y);
                    }
                    map[x][y]->adj[opposite(lastStep)] = currentNode;                       // exchanges info between the two nodes
                    currentNode->adj[d] = map[x][y];
                    goto end;               // Please forgive my sin, I only wanted to skip the backtracking code
                } else {
                    travel(opposite(lastStep));             // if dead end then retreat
                    currentNode->adj[d] = &DEAD_END;
                }
            }
        }
        s.pop();                        // if no direction yielded a node (i.e. all directions were dead ends), this node is effectively a dead end too
        travel(s.top());                // backtrack
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
    }

}
