#include <iostream>
#include <vector>
#include <algorithm>
#include <utility> 
#include <cmath>
#include <chrono> 

//1，partition by hmetis and build the slicing tree
//The circuit_subs obtained after partitioning the original circuit with hmetis are used as input, and N shapes are included in each circuit_sub
//Construct slicing tree. The parent node of slicing tree is the undivided original circuit, the tree node is the subpartition generated in the process of partitioning, and the leaf node is the circuit_sub obtained after partitioning

struct shape{
    double w; 
	double h; 
	
    shape(double w = 0, double h = 0) : w(w), h(h){}
};


//struct circuit_sub {
//    std::vector<shape> shapes;
//    circuit_sub(int N) {
//        shapes.reserve(N);
//    }
//};

//std::vector<std::pair<double, double>> shape_data;  //shape is a vector of dimension 1x2 containing the variables w,h, which mean the width and height of the shape
//std::vector<shape> circuit_ini; //circuit_ini is an Mx2 dimensional matrix, consisting of M shapes containing M/N circuit_subs
//std::vector<shape> circuit_subs;  //A circuit_sub is an Nx2 dimensional matrix, consisting of N shapes
//std::vector<shape> circuit;

//
//// bisection 
//std::pair<circuit_sub, circuit_sub> bisection(const circuit_sub& circuit, int N) {
//    circuit_sub left(N), right(N);
//    size_t mid = circuit.shapes.size() / 2;
//    for (size_t i = 0; i < mid; ++i) {
//        left.shapes.push_back(circuit.shapes[i]);
//    }
//    for (size_t i = mid; i < circuit.shapes.size(); ++i) {
//        right.shapes.push_back(circuit.shapes[i]);
//    }
//    return {left, right};
//}
//
////The hmetisfunc function is constructed, which makes it possible to recursively bisect the original circuit until each leaf node contains N shapes
//void hmetisfunc(std::vector<shape>& circuit_ini, int N, std::vector<shape>& circuit_sub){
//	if (circuit_ini.size() <= N) {
//	shape s;
//	circuit_sub.push_back(s);
//	return;
//	}
//
//	circuit_sub s;
//	auto [left, right] = bisection(s, N);
//	hmetisfunc(left.shapes, N, circuit_sub);
//	hmetisfunc(right.shapes, N, circuit_sub);
//	
//}
//
//
//struct Node {
//    int val;
//    Node* left;
//    Node* right;
//    Node(int x) : val(x), left(NULL), right(NULL) {}
//};
//
//
//void insert(Node* &root, int val, bool isLeft) {
//    if (root == NULL) {
//        root = new Node(val);
//    } else {
//        if (isLeft) {
//            if (root->left == NULL) {
//                root->left = new Node(val);
//            } else {
//                insert(root->left, val, isLeft);
//            }
//        } else {
//            if (root->right == NULL) {
//                root->right = new Node(val);
//            } else {
//                insert(root->right, val, isLeft);
//            }
//        }
//    }
//}
//
//Node* Tree(std::vector<shape>& shapes, int N) {
//    Node* root = NULL; 
//    insert(root, 1, true); 
//    insert(root->left, 2, true); 
//    insert(root->right, 3, false);
//    insert(root->left->left, 4, true);
//    insert(root->left->right, 5, false);
//    insert(root->right->left, 6, true);
//    insert(root->right->right, 7, false);
//    return root;
//}




typedef std::vector<std::pair<double, double>> Points;
//Points curve(const std::pair<double, double>& shape, int num_points=100);

Points curve(const shape& shape, int num_points) {
    Points curvePoints;

    double x_min = 0.5;
    double x_max = 20;
    double step = (x_max - x_min) / (num_points - 1);

    for (int i = 0; i < num_points; ++i) {
        double x = x_min + i * step;
        double y = shape.w * shape.h / x;

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
        Cv.push_back(std::make_pair(pointCh.second, pointCh.first)); 
    }
    return Cv;
}

//merging
Points merging(const Points& curveCh, const Points& curveCv) {
    Points C;
    auto itCh = curveCh.begin();
    auto itCv = curveCv.begin();
	double y_1 = 0;
	
    while (itCh != curveCh.end() && itCv != curveCv.end()) {
        if (itCh->second == itCv->second) {
            y_1 = itCh->second; 
            
        }
    }

    while (itCh != curveCh.end() && itCv != curveCv.end()) {
        double x_ch = itCh->first;
        double y_ch = itCh->second;
        double x_cv = itCv->first;
        double y_cv = itCv->second;

        if (y_ch == y_1) {
            C.push_back({x_ch, y_ch}); 
        } 
		else if (y_ch < y_1) {
            C.push_back({x_ch, y_ch}); 
            ++itCh;
        } 
		else {
            C.push_back({x_cv, y_cv}); 
            ++itCv;
        }
    }
    return C;
}




//    while (itCh != curveCh.end() && itCv != curveCv.end()) {
//        double x_ch = itCh->first;
//        double y_ch = itCh->second;
//        double x_cv = itCv->first;
//        double y_cv = itCv->second;
//
//        if (y_ch == y_cv) {
//            y_1 = y_ch; // 更新y_1为当前纵坐标相等的值
//            C.push_back({x_ch, y_ch}); // 添加Ch的点
//            ++itCh;
//            ++itCv;
//        } else if (y_ch < y_1) {
//            C.push_back({x_ch, y_ch}); // y_ch小于y_1，添加Ch的点
//            ++itCh;
//        } else {
//            C.push_back({x_cv, y_cv}); // y_ch大于y_1，添加Cv的点
//            ++itCv;
//        }
//    }
    

//	while(itCh->second==itCv->second){
//		double x_1 = itCh->first;	
//	}

//      if(x_c = x_ch){
//			C.push_back({x_ch, y_ch});
//			++itCh;
//			
//		}
//	
//		else{
//			C.push_back({x_cv, y_cv});
//		}
//    
//    while (itCh != curveCh.end() && itCv != curveCv.end()) {
//        double x_ch = itCh->first;
//        double y_ch = itCh->second;
//            
//        while (itCv->second < y_ch) {
//            itCv++; 
//        }
//
//        double x_cv = itCv->first;
//        if (x_cv < x_ch) {
//            C.push_back({x_cv, y_ch});
//        } 
//        else {
//            C.push_back({x_ch, y_ch});
//        }   
//
//        ++itCh; 
//    }



//
////merging
//Points merging(const Points& curveCh, const Points& curveCv) {
//    Points C;
//    auto itCh = curveCh.begin();
//    auto itCv = curveCv.begin();
//
//    while(curveCh.begin()->second < curveCv.begin()->second) {
//        while(itCh->second < curveCv.begin()->second){
//            C.push_back({itCh->first, itCh->second});
//            ++itCh;
//        }
//        while(itCh == curveCh.end() && curveCh.end()->second < curveCv.end()->second){
//            C.push_back({itCv->first, itCv->second});
//            ++itCv;
//        }
//    }
//
//    while(curveCh.begin()->second > curveCv.begin()->second) {
//        while(itCv->second < curveCh.begin()->second){
//            C.push_back({itCv->first, itCv->second});
//            ++itCv;
//        }
//        while(itCv == curveCv.end() && curveCv.end()->second < curveCh.end()->second){
//            C.push_back({itCh->first, itCh->second});
//            ++itCh;
//        }
//    }
//
//    while (itCh != curveCh.end() && itCv != curveCv.end()) {
//        double x_ch = itCh->first;
//        double y_ch = itCh->second;
//            
//        while (itCv->second < y_ch) {
//            itCv++; 
//        }
//
//        double x_cv = itCv->first;
//        double x_c = fmin(x_cv, x_ch );
//        C.push_back({x_c, y_ch});
//
//        ++itCh; 
//    }
//
//    return C;
//}
//
//


//
//
//// //merging
//Points merging(const Points& curveCh, const Points& curveCv) {
//    Points C;
//    auto itCh = curveCh.begin();
//    auto itCv = curveCv.begin();
//
//    if(curveCh.begin()->second > curveCv.begin()->second){
//        while(itCv->second < curveCh.begin()->second){
//            C.push_back({itCv->first, itCv->second}); 
//            ++itCv;
//        }
//
//        while (itCh != curveCh.end()) {
//            double x_ch = itCh->first;
//            double y_ch = itCh->second;
//            
//            while (itCv != curveCv.end() && itCv->second < y_ch) {
//                itCv++; 
//            }
//
//            double x_cv = itCv->first;
//            if (x_cv < x_ch) {
//                C.push_back({x_cv, y_ch});
//            } 
//            else {
//                C.push_back({x_ch, y_ch});
//            }   
//
//            if(itCv == curveCv.end() && curveCh.end()->second > curveCv.end()->second){
//                C.push_back({itCh->first, itCh->second});
//                ++itCh;
//            }
//
//            ++itCh; 
//            }
//    }
//
//    if (curveCh.begin()->second < curveCv.begin()->second) {
//        while(itCh->second < curveCv.begin()->second){
//            C.push_back({itCh->first, itCh->second});
//            ++itCh;
//        }
//        while (itCh != curveCh.end()) {
//            double x_ch = itCh->first;
//            double y_ch = itCh->second;
//            
//            while (itCv != curveCv.end() && itCv->second < y_ch) {
//                itCv++; 
//            }
//
//            double x_cv = itCv->first;
//            if (x_cv < x_ch) {
//                C.push_back({x_cv, y_ch});
//            } 
//            else {
//                C.push_back({x_ch, y_ch});
//            }   
//
//            if(itCh == curveCv.end() && curveCh.end()->second < curveCv.end()->second){
//                C.push_back({itCv->first, itCv->second});
//                ++itCv;
//            }
//
//            ++itCh; 
//            }
//    }
//
//    return C;
//}

//print the curve points of C
void printcurve(const Points& points, int num_points=100) {
    for (int i = 0; i < num_points && i < points.size(); ++i) {
        const auto& point = points[i];
//        std::cout << "Point " << i << ": (" << point.first << ", " << point.second << ")" << std::endl;
        std::cout << point.first << "   " << point.second << std::endl;
    }
}




//main function
int main() {
	auto start = std::chrono::high_resolution_clock::now(); 
    double wA, hA, wB, hB;
    std::cout << "Enter width and height for Module A (wA, hA): ";
    std::cin >> wA >> hA;
    std::cout << "Enter width and height for Module B (wB, hB): ";
    std::cin >> wB >> hB;

    shape shapeA(wA, hA);
    shape shapeB(wB, hB);
    Points curveA = curve(shapeA, 100);
    Points curveB = curve(shapeB, 100);

    Points curveCh = addition(curveA, curveB);

    Points curveCv = flipping(curveCh);

    Points curveC = merging(curveCh, curveCv);

    std::cout << "Curve A points:" << std::endl;
    printcurve(curveA, 100);
    
    std::cout << "Curve B points:" << std::endl;
    printcurve(curveB, 100);
    
    std::cout << "Curve Ch points:" << std::endl;
    printcurve(curveCh, 100);

    std::cout << "Curve Cv points:" << std::endl;
    printcurve(curveCv, 100);

    std::cout << "Curve C points:" << std::endl;
    printcurve(curveC, 100);


    auto stop = std::chrono::high_resolution_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start); 

    std::cout << "Runtime: " << duration.count() << " seconds" << std::endl;

    return 0;
}










