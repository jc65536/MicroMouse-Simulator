
#include "micromouseserver.h"
#include <iostream>
#include <stack>
#include <map>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <queue>

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

struct Node;

struct Pair {
    Node *node;
    int d;
};

// a node is any tile with more than 2 exit paths
struct Node {
    int i, x, y;
    std::map<Dir, Pair> adj;
    int d = INT_MAX;
    Node *prev;
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
    static bool newRun = true;              // reset every run
    static bool firstRun = true;            // only true for the first run - how do I make this reset every map change?

    // x: mouse current x position; y: mouse current y position; nodeNum: just a counter to identify each node when I print them later
    static int x, y, nodeNum;
    static Dir lastStep;                    // always updated to be the same direction as the step we just took

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
                return 0;
            default:                                            // reached node (crossroads)
                std::cout << "reached node " << x << ", " << y << std::endl;
                return steps;
            }
        }
    };

    static Node *map[20][20];               // map[x][y] will tell us if we are at an existing node or an unvisited tile, since map is cleared to 0 by default
    static std::stack<Dir> s;               // use this stack for backtracking
    static Node *rootNode;
    static bool exploring = true;
    static std::stack<Dir> optimalPath;
    static std::stack<Dir> pathCopy;

    if (newRun) {
        x = y = 0;
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
        memset(map, 0, 400 * sizeof(Node *));
        map[x][y] = new Node(nodeNum++, x, y);              // set the first starting node
        rootNode = map[x][y];
        rootNode->d = 0;
        pathCopy = optimalPath;                             // load optimal path on new run
        newRun = false;
    }

    if (firstRun) {
        if (exploring) {                    // explore all nodes
            Node *currentNode = map[x][y];
            int paths = test();

            for (int i = 0; i < 4; i++) {
                Dir d = Dir(1 << i);
                if (paths & d) {
                    if (!currentNode->adj[d].node) {
                        int t = travel(d);
                        if (t) {
                            if (map[x][y]) {
                                map[x][y]->adj[opposite(lastStep)] = {currentNode, t};               // exchanges info between the two nodes
                                currentNode->adj[d] = {map[x][y], t};
                                travel(opposite(lastStep));
                            } else {
                                std::cout << "new node created" << std::endl;
                                map[x][y] = new Node(nodeNum++, x, y);
                                map[x][y]->adj[opposite(lastStep)] = {currentNode, t};
                                currentNode->adj[d] = {map[x][y], t};
                                s.push(opposite(lastStep));                                         // push opposite of last step for backtracking
                                goto end;
                            }
                        } else {
                            travel(opposite(lastStep));                                             // if dead end then retreat
                            currentNode->adj[d] = {&DEAD_END, 0};
                        }
                    }
                } else {
                    currentNode->adj[d] = {&DEAD_END, 0};
                }
            }

            if (currentNode == rootNode) {
                exploring = false;
                goto end;
            }
            std::cout << "going back" << std::endl;
            travel(s.top());                // backtrack
            s.pop();
            if ((currentNode->adj[N].node == &DEAD_END) + (currentNode->adj[E].node == &DEAD_END) + (currentNode->adj[W].node == &DEAD_END) + (currentNode->adj[S].node == &DEAD_END) == 3) {
                std::cout << "this node dead" << std::endl;
                map[currentNode->x][currentNode->y] = &DEAD_END;              // a node with 3 dead ends is effectively a dead end itself
                map[x][y]->adj[opposite(lastStep)] = {&DEAD_END, 0};
            }

end:
            for (int i = 0; i < 20; i++) {
                for (int j = 0; j < 20; j++) {
                    if (map[i][j])
                        std::cout << std::setw(3) << map[i][j]->i;
                    else
                        std::cout << " . ";
                }
                std::cout << std::endl;
            }
        } else {                           // calculate optimal path
            std::queue<Node *> q;
            q.push(rootNode);
            bool visited[20][20];
            memset(visited, false, 400 * sizeof(bool));

            while (q.size() > 0) {
                Node *currentNode = q.front();
                visited[currentNode->x][currentNode->y] = true;
                for (int i = 0; i < 4; i++) {
                    Pair p = currentNode->adj[Dir(1 << i)];
                    if (p.node != &DEAD_END) {
                        if (currentNode->d + p.d < p.node->d) {
                            p.node->d = currentNode->d + p.d;
                            p.node->prev = currentNode;
                        }
                        if (!visited[p.node->x][p.node->y]) {
                            q.push(p.node);
                        }
                    }
                }
                q.pop();
            }

            Node *n = map[DX][DY];
            while (n != rootNode) {
                std::cout << n->i << "<-" << n->prev->i << " (" << n->d << ")" << std::endl;
                for (int i = 0; i < 4; i++) {
                    Dir d = Dir(1 << i);
                    if (n->prev->adj[d].node == n)
                        optimalPath.push(d);
                }
                n = n->prev;
            }
            pathCopy = optimalPath;
            firstRun = false;
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
