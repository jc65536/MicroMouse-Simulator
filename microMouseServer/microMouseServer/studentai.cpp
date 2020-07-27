
#include "micromouseserver.h"
#include <iostream>
#include <stack>
#include <map>
#include <cstring>

const int DX = 10;
const int DY = 6;

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

    int x = 0, y = 0, nodeNum = 0;
    Dir lastStep;

    // returns 1 for open directions and 0 for blocked directions
    auto test = [&]() -> int {
        int r = isWallForward() + isWallRight() * 2 + isWallLeft() * 4;
        turnRight();
        r += isWallRight() * 8;
        turnLeft();
        r ^= 0b1111;
        return r;
    };

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

    /* Pseudocode
        start at a node
        set going back to false
        while not at destination:
            if not going back:
                push opposite of last step to backtrack stack
            else:
                mark opposite of last step as dead end
            set going back to false
            set paths to test value
            for each available direction d:
                if d is not a dead end:
                    travel in direction d
                    if dead end:
                        go back
                        mark direction d as dead end
                    else:
                        if node doesn't exist:
                            make a new node
                        add the node we just came from to this node's adj list
                        add this node to the adj list of the node we just came from
                        add adjacents like above
                        break for, continue
            if all directions are dead ends
                pop from backtrack stack
                peek direction d from backtrack stack
                go back in direction d
                set going back to true
     */

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

    Node *map[20][20];
    std::memset(map, 0, 400 * sizeof(Node *));
    std::stack<Dir> s;
    map[x][y] = new Node(nodeNum++, x, y);

    bool goingBack = false;
    while (x != DX && y != DY) {
        Node *currentNode = map[x][y];

        if (!goingBack)
            s.push(opposite(lastStep));
        else
            currentNode->adj[opposite(lastStep)] = &DEAD_END;
        goingBack = false;

        int paths = test();
        for (int i = 0; i < 4; i++) {
            Dir d = Dir(1 << i);
            if (paths & d && currentNode->adj[d] != &DEAD_END && d != s.top()) {
                bool t = travel(d);
                if (t) {
                    if (!map[x][y]) {
                        std::cout << "new node created" << std::endl;
                        map[x][y] = new Node(nodeNum++, x, y);
                    }
                    map[x][y]->adj[opposite(lastStep)] = currentNode;
                    currentNode->adj[d] = map[x][y];
                    goto end;
                } else {
                    travel(opposite(lastStep));
                    currentNode->adj[d] = &DEAD_END;
                }
            }
        }
        s.pop();
        travel(s.top());
        goingBack = true;

    end:
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
