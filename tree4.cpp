#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <utility> 

//1，partition by hmetis and build the slicing tree
//The subcircuits obtained after partitioning the original circuit with hmetis are used as input, and N modules are included in each subcircuit
//Construct slicing tree. The parent node of slicing tree is the undivided original circuit, the tree node is the subpartition generated in the process of partitioning, and the leaf node is the subcircuit obtained after partitioning

struct Module{
    double w; 
	double h; 
	
    Module(double w = 0, double h = 0) : w(w), h(h){}
};


struct SubCircuit {
    std::vector<Module> modules;
    SubCircuit(int N) {
        modules.reserve(N);
    }
};


template pair make_pair(T1 w, T2 h) { 
	return pair(w, h);
}


std::vector<std::pair<double, double>> Module; //Module is a vector of dimension 1x2 containing the variables w,h, which mean the width and height of the module
std::vector<Module> IniCircuit;  //IniCircuit is an Mx2 dimensional matrix, consisting of M modules containing M/N subcircuits
std::vector<Module> SubCircuit;  //A SubCircuit is an Nx2 dimensional matrix, consisting of N modules
std::vector<Module> Circuit;  

// bisection 
std::pair<SubCircuit, SubCircuit> bisection(const SubCircuit& Circuit, int N) {
    SubCircuit left(N), right(N);
    size_t mid = Circuit.modules.size() / 2;
    for (size_t i = 0; i < mid; ++i) {
        left.modules.push_back(Circuit.modules[i]);
    }
    for (size_t i = mid; i < Circuit.modules.size(); ++i) {
        right.modules.push_back(Circuit.modules[i]);
    }
    return {left, right};
}

//The hmetisfunc function is constructed, which makes it possible to recursively bisect the original circuit until each leaf node contains N modules
void hmetisfunc(std::vector<Module>& IniCircuit, int N, std::vector<Module>& SubCircuit){
	if (IniCircuit.size() <= N) {
	SubCircuit.push_back(IniCircuit);
	return;
	}
	auto [left, right] = bisection(const SubCircuit& IniCircuit, int N);
	hmetisfunc(left.modules, N, SubCircuit);
	hmetisfunc(right.modules, N, SubCircuit);
}


struct Node {
    int val;
    Node* left;
    Node* right;
    Node(int x) : val(x), left(NULL), right(NULL) {}
};


void insertleft(Node* &root, int val) {
    if (root == NULL) { 
        root = new Node(val);
    } else {
        if (root->left == NULL) { 
            root->left = new Node(val);
        } else { 
            insertleft(root->left, val);
        }
    }
}


void insertright(Node* &root, int val) {
    if (root == NULL) { 
        root = new Node(val);
    } else {
        if (root->right == NULL) { 
            root->right = new Node(val);
        } else { 
            insertright(root->right, val);
        }
    }
}


Node* Tree(std::vector<Module>& modules, int N) {
    Node* root = NULL; 
    insertleft(root, 1); 
    insertleft(root->left, 2); 
    insertright(root->right, 3);
    insertleft(root->left->left, 4);
    insertright(root->left->right, 5);
    insertleft(root->right->left, 6);
    insertright(root->right->right, 7);
    return root;
}


//2，combine the curve and shape the merge curve

typedef std::vector<std::pair<double, double>> Points;
Points curve(const std::pair<double, double>& module, int num_points=100);

Points curve(const Module& module, int num_points=100) {
    Points curvePoints;
    double x_min, x_max, step;

    double x_min = std::min(module.first,module.second);
    double x_max = std::max(module.first,module.second);
    double step = (x_max - x_min) / (num_points - 1);

    for (int i = 0; i < num_points; ++i) {
        double x = x_min + i * step;
        double y = module.first * module.second / x;

        curvePoints.emplace_back(x, y);
    }

    return curvePoints;
}


//addition
Points addition(const Points& curveA, const Points& curveB) {
    Points Ch;  
    for (const auto& pointA : curveA) {
        for (const auto& pointB : curveB) {
            double newx = pointA.x + pointB.x;
            double newy = pointA.y;
            Ch.push_back(std::make_pair(newx, newy));  
        }
    }
    return Ch;
}


//flipping
Points flipping(const Points& curveCh) {
    Points Cv;  
    for (const auto& pointCh : curveCh) {
        Cv.push_back(module(pointCh.y, pointCh.x)); 
    }
    return Cv;
}


bool findXForY(const Points& curve, double y, double& x) {
    for (const auto& point : curve) {
        if (point.second == y) {
            x = point.first;
            return true;
        }
    }
    return false;
}

//merging
Points merging(const Points& curve, const Points& curveCh, const Points& curveCv, int num_points=100) {
    Points C; 
    Points allPoints;
    allPoints.reserve(curveCh.size() + curveCv.size()); 

    std::merge(curveCh.begin(), curveCh.end(), curveCv.begin(), curveCv.end(), std::back_inserter(allPoints),
              [](const std::pair<double, double>& a, const std::pair<double, double>& b) {
                  return a.second < b.second;
              });
    
    double x_min = std::min(curveCh.first, curveCv.first);
    double x_max = std::max(curveCh.second, curveCh.second);
    double step = (x_max - x_min) / (num_points - 1);
    double x_last = -1; 
    double y_last = -1; 

    for (double y = y_min; y <= y_max; y += step) {
        double curveC.second = y;
        double x = min(findXForY(curveCh, y, x), findXForY(curveCv, y, x)); 

        if ( x < x_last ) {
            curveC.first = x;
            curveC.push_back({x, y}); 
        }
    }

    return C;
}


//print the curve points of C
void printcurve(const Points& points, int num_points=100) {
    for (int i = 0; i < num_points && i < points.size(); ++i) {
        const auto& point = points[i];
        std::cout << "Point " << i << ": (" << point.first << ", " << point.second << ")" << std::endl;
    }
}


//main function: Input the initial circuit, call the hmetis function for recursive bisection, get the SubCircuit containing N modules, and generate its shapecurve curve
int main() {
    const size_t M = 4; // IniCircuit is the matrix of Mx2
    const size_t N = 2;  // SubCircuit is the matrix of Nx2
	
    for (int i = 0; i < M; ++i) {
    	double w, h;
        std::cout << "Enter w, h for module " << i + 1 << ": ";
        std::cin >> w >> h ;
        IniCircuit.emplace_back(w, h);
    }


    hmetisfunc(IniCircuit, N, SubCircuit);

    std::cout << "SubCircuits:" << std::endl;
    for (const auto& Sub : SubCircuits) {
        for (const auto& mod : Sub.modules) {
            std::cout << "(" << mod.w << ", " << mod.h << ") ";
        }
        std::cout << std::endl;
    }
    
    Points curveC = merging(curveCh, curveCv);
    printcurve(curveC, 100);

    return 0;
}


//还在修改中
