#include <iostream>
#include <vector>
#include <algorithm>
#include <utility> 
#include <cmath>

//1，partition by hmetis and build the slicing tree
//The circuit_subs obtained after partitioning the original circuit with hmetis are used as input, and N shapes are included in each circuit_sub
//Construct slicing tree. The parent node of slicing tree is the undivided original circuit, the tree node is the subpartition generated in the process of partitioning, and the leaf node is the circuit_sub obtained after partitioning

struct shape{
    double w; 
	double h; 
	
    shape(double w = 0, double h = 0) : w(w), h(h){}
};


struct circuit_sub {
    std::vector<shape> shapes;
    circuit_sub(int N) {
        shapes.reserve(N);
    }
};


std::vector<std::pair<double, double>> shape; //shape is a vector of dimension 1x2 containing the variables w,h, which mean the width and height of the shape
std::vector<shape> circuit_ini;  //circuit_ini is an Mx2 dimensional matrix, consisting of M shapes containing M/N circuit_subs
std::vector<shape> circuit_sub;  //A circuit_sub is an Nx2 dimensional matrix, consisting of N shapes
std::vector<shape> circuit;  

// bisection 
std::pair<circuit_sub, circuit_sub> bisection(const circuit_sub& circuit, int N) {
    circuit_sub left(N), right(N);
    size_t mid = circuit.shapes.size() / 2;
    for (size_t i = 0; i < mid; ++i) {
        left.shapes.push_back(circuit.shapes[i]);
    }
    for (size_t i = mid; i < circuit.shapes.size(); ++i) {
        right.shapes.push_back(circuit.shapes[i]);
    }
    return {left, right};
}

//The hmetisfunc function is constructed, which makes it possible to recursively bisect the original circuit until each leaf node contains N shapes
void hmetisfunc(std::vector<shape>& circuit_ini, int N, std::vector<shape>& circuit_sub){
	if (circuit_ini.size() <= N) {
	circuit_sub.push_back(circuit_ini);
	return;
	}
    auto [left, right] = bisection(circuit_ini, N);
	hmetisfunc(left.shapes, N, circuit_sub);
	hmetisfunc(right.shapes, N, circuit_sub);
}


struct Node {
    int val;
    Node* left;
    Node* right;
    Node(int x) : val(x), left(NULL), right(NULL) {}
};


void insert(Node* &root, int val, bool isLeft) {
    if (root == NULL) {
        root = new Node(val);
    } else {
        if (isLeft) {
            if (root->left == NULL) {
                root->left = new Node(val);
            } else {
                insert(root->left, val, isLeft);
            }
        } else {
            if (root->right == NULL) {
                root->right = new Node(val);
            } else {
                insert(root->right, val, isLeft);
            }
        }
    }
}

Node* Tree(std::vector<shape>& shapes, int N) {
    Node* root = NULL; 
    insert(root, 1, true); 
    insert(root->left, 2, true); 
    insert(root->right, 3, false);
    insert(root->left->left, 4, true);
    insert(root->left->right, 5, false);
    insert(root->right->left, 6, true);
    insert(root->right->right, 7, false);
    return root;
}


//2，combine the curve and shape the merge curve

typedef std::vector<std::pair<double, double>> Points;
Points curve(const std::pair<double, double>& shape, int num_points=100);

Points curve(const shape& shape, int num_points=100) {
    Points curvePoints;
    double x_min, x_max, step;

    double x_min = fmin(shape.w,shape.h);
    double x_max = fmax(shape.w,shape.h);
    double step = (x_max - x_min) / (num_points - 1);

    for (int i = 0; i < num_points; ++i) {
        double x = x_min + i * step;
        double y = shape.first * shape.second / x;

        curvePoints.emplace_back(x, y);
    }

    return curvePoints;
}


//addition
Points addition(const Points& curveA, const Points& curveB) {
    Points Ch;
    auto itA = curveA.begin();
    auto itB = curveB.begin();

    while (itA != curveA.end()) {
        double x_a = itA->first;
        double y_a = itA->second;
        
        auto itB_upper = curveB.begin();
        while (itB_upper != curveB.end() && itB_upper->second < y_a) {
            itB_upper++; 
        }

        auto itB_lower = itB_upper;
        if (itB_upper != curveB.begin()) {
            itB_lower--;  
        }

        double x_b1 = itB_upper->first;
        double x_b2 = itB_lower->first;

        double x_c1 = x_a + x_b1;
        double y_c1 = y_a;  
        double x_c2 = x_a + x_b2;
        double y_c2 = y_a;  

        double area1 = x_c1 * y_c1;
        double area2 = x_c2 * y_c2;

        if (area1 < area2) {
            Ch.push_back({x_c1, y_c1});
        } 
        else {
            Ch.push_back({x_c2, y_c2});
        }

        ++itA; 
    }

    return Ch;
}



//flipping
Points flipping(const Points& curveCh) {
    Points Cv;  
    for (const auto& pointCh : curveCh) {
        Cv.push_back(std::make_pair(pointCh.y, pointCh.x)); 
    }
    return Cv;
}



//merging
Points merging(const Points& curveCh, const Points& curveCv) {
    Points C;
    auto itCh = curveCh.begin();
    auto itCv = curveCv.begin();

    while(curveCh.begin()->second < curveCv.begin()->second) {
        while(itCh->second < curveCv.begin()->second){
            C.push_back({itCh->first, itCh->second});
            ++itCh;
        }
        while(itCh = curveCh.end() && curveCh.end()->second < curveCv.end()->second){
            C.push_back({itCv->first, itCv->second});
            ++itCv;
        }
    }

    while(curveCh.begin()->second > curveCv.begin()->second) {
        while(itCv->second < curveCh.begin()->second){
            C.push_back({itCv->first, itCv->second});
            ++itCv;
        }
        while(itCv = curveCv.end() && curveCv.end()->second < curveCh.end()->second){
            C.push_back({itCh->first, itCh->second});
            ++itCh;
        }
    }

    while (itCh != curveCh.end() && itCv != curveCv.end()) {
        double x_ch = itCh->first;
        double y_ch = itCh->second;
            
        while (itCv->second < y_ch) {
            itCv++; 
        }

        double x_cv = itCv->first;
        double x_c = fmin(x_cv, x_ch );
        C.push_back({x_c, y_ch});

        ++itCh; 
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


//main function: Input the initial circuit, call the hmetis function for recursive bisection, get the circuit_sub containing N shapes, and generate its shapecurve curve
int main() {
    const size_t M = 4; // circuit_ini is the matrix of Mx2
    const size_t N = 2;  // circuit_sub is the matrix of Nx2
	
    for (int i = 0; i < M; ++i) {
    	double w, h;
        std::cout << "Enter w, h for shape " << i + 1 << ": ";
        std::cin >> w >> h ;
        circuit_ini.emplace_back(w, h);
    }

    hmetisfunc(circuit_ini, N, circuit_sub);
    for (const auto& sub : circuit_sub) {
        curveCh = curve(sub.shapes[0], 100);  
        curveCv = curve(sub.shapes[1], 100);  
    }
    Points curveCh, curveCv;
    Points curveC = merging(curveCh, curveCv);
    printcurve(curveC, 100);

    return 0;
}


//还在修改中





