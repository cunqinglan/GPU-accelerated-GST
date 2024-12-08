#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <functional>
#include <memory> // for std::unique_ptr
#include <cstdlib>  // for rand and srand
#include <ctime>    // for seeding rand

// Define the module structure to store the shape curve modules
struct module {
    double width;  // Width of the point
    double height; // Height of the point

    module(double w = 0, double h = 0) : width(w), height(h) {}

    bool operator<(const module& other) const {
        if (height == other.height) {
            return width < other.width; // Sort by width
        }
        return height < other.height; // Sort by height
    }
};

// Define ShapeCurveRange to store the range of the curve
struct ShapeCurveRange {
    double W_min, W_max, H_min, H_max;  // Range of the curve in x and y directions

    ShapeCurveRange(double WMin, double WMax, double HMin, double HMax)
        : W_min(WMin), W_max(WMax), H_min(HMin), H_max(HMax) {}

    // Function to check if a module is inside the ShapeCurve's range
    bool isInRange(const module& m) const {
        return m.width >= W_min && m.width <= W_max && m.height >= H_min && m.height <= H_max;
    }
};


// Define SlicingTreeNode type to represent nodes of the Slicing Tree
struct SlicingTreeNode {
    std::vector<std::unique_ptr<SlicingTreeNode>> children;  
    std::vector<module> shapeCurve;
    bool isHorizontal;
    bool isSubcircuit;
    
    SlicingTreeNode() : isHorizontal(false), isSubcircuit(false) {}
};


// Define ShapeCurve type to store shape curve modules
using ShapeCurve = std::vector<module>;

// Function declarations for the functions defined later
std::vector<std::vector<module>> hMetisPartition(const std::vector<module>& modules, int maxN);
ShapeCurve enumerativePacking(const std::vector<module>& modules);
ShapeCurve combineShapeCurves(const SlicingTreeNode* node);
ShapeCurve mergeCurves(const ShapeCurve& curveA, const ShapeCurve& curveB);
ShapeCurve addCurvesHorizontally(const ShapeCurve& curveA, const ShapeCurve& curveB);
ShapeCurve flipCurveVertically(const ShapeCurve& curve);
std::unique_ptr<SlicingTreeNode> buildSlicingTree(std::vector<module>& modules, int maxN);

// Hypothetical hMetis partition function, recursively bisecting until the number of modules is less than or equal to maxN
std::vector<std::vector<module>> hMetisPartition(const std::vector<module>& modules, int maxN = 10) {
    std::vector<std::vector<module>> partitions;

    // Base partition condition: when the number of modules is less than or equal to maxN, return directly
    if (modules.size() <= maxN) {
        partitions.push_back(modules);
        return partitions;
    }

    // Simplified recursive bisecting: partition by width
    size_t mid = modules.size() / 2;
    std::vector<module> left(modules.begin(), modules.begin() + mid);
    std::vector<module> right(modules.begin() + mid, modules.end());

//    // Recursively partition the left and right parts
//    auto leftPartitions = hMetisPartition(left, maxN);
//    auto rightPartitions = hMetisPartition(right, maxN);
//
//    // Merge the left and right partitions
//    partitions.insert(partitions.end(), leftPartitions.begin(), leftPartitions.end());
//    partitions.insert(partitions.end(), rightPartitions.begin(), rightPartitions.end());

    return partitions;
}

// Enumerative packing - generate all possible cut layouts
ShapeCurve enumerativePacking(const std::vector<module>& modules) {
    std::set<module> resultSet; // Use set to avoid duplicates
    for (size_t i = 0; i < modules.size(); i++) {
        for (size_t j = i + 1; j < modules.size(); j++) {
            double newWidth = modules[i].width + modules[j].width;
            double newHeight = std::max(modules[i].height, modules[j].height);
            resultSet.insert(module(newWidth, newHeight));
        }
    }
    // Return the set as a vector
    return ShapeCurve(resultSet.begin(), resultSet.end());
}

// Merge two subplane curves (based on "⊕" operation)
ShapeCurve combineShapeCurves(const SlicingTreeNode* node) {
    if (node->children.empty()) {
        return node->shapeCurve; // If it's a leaf node, directly return the curve
    }

    // Recursively merge the curves of child nodes
    ShapeCurve combinedCurve;
    if (!node->children.empty()) {
        combinedCurve = combineShapeCurves(node->children[0].get());
    }
    if (node->children.size() > 1) {
        auto rightCurve = combineShapeCurves(node->children[1].get());
        combinedCurve = mergeCurves(combinedCurve, rightCurve);
    }

    return combinedCurve;
}

// Horizontal addition operation: combine two curves in the horizontal direction
ShapeCurve addCurvesHorizontally(const ShapeCurve& curveA, const ShapeCurve& curveB) {
    ShapeCurve Ch;  // Define the curve after horizontal addition
    for (const auto& pointA : curveA) {
        for (const auto& pointB : curveB) {
            // New width is the sum of the widths, and the height is the maximum of both heights
            double newWidth = pointA.width + pointB.width;
            double newHeight = std::max(pointA.height, pointB.height);
            Ch.push_back(module(newWidth, newHeight));  // Add the resulting module to Ch
        }
    }
    return Ch;
}

// Flip operation: flip the curve along the W=H line
ShapeCurve flipCurveVertically(const ShapeCurve& curve) {
    ShapeCurve Cv;  // Define the flipped curve
    for (const auto& point : curve) {
        // Flip each module by swapping width and height
        Cv.push_back(module(point.height, point.width));  // The width becomes height, and the height becomes width
    }
    return Cv;
}


bool compareByHeight(const module& a, const module& b) {
    return a.height < b.height;
}

std::vector<module> mergeCurves(const std::vector<module>& Ch, const std::vector<module>& Cv) {
    std::vector<module> allPoints = Ch;
    allPoints.insert(allPoints.end(), Cv.begin(), Cv.end());

    std::sort(allPoints.begin(), allPoints.end(), compareByHeight);

    std::vector<module> finalCurve;
    double lastWidth = -1;  
    double lastHeight = -1;

    for (size_t i = 0; i < allPoints.size(); ++i) {
        const module& current = allPoints[i];

        if (i == 0 || current.height != allPoints[i - 1].height) {
            finalCurve.push_back(current);
            lastWidth = current.width;
            lastHeight = current.height;
        } else {
            if (current.width < lastWidth) {
                finalCurve.back() = current;  
                lastWidth = current.width;
                lastHeight = current.height;
            }
        }

        if (lastWidth < current.width && current.height > lastHeight) {
            continue;
        }
    }

    return finalCurve;
}


void printCurve(const std::vector<module>& curve) {
    for (const auto& point : curve) {
        std::cout << "(" << point.width << ", " << point.height << ") ";
    }
    std::cout << std::endl;
}


std::unique_ptr<SlicingTreeNode> buildSlicingTree(std::vector<module>& modules, int maxN = 10) {
    auto partitions = hMetisPartition(modules, maxN);
    auto root = std::make_unique<SlicingTreeNode>();
    for (auto& partition : partitions) {
        auto childNode = std::make_unique<SlicingTreeNode>();
        childNode->shapeCurve = enumerativePacking(partition);
        childNode->isSubcircuit = true;
        root->children.push_back(std::move(childNode));  
    }
    return root;
}


int main() {
    // Seed the random number generator
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Randomly generate 1000 sample modules with defined width W and height H
    std::vector<module> modules;
    for (int i = 0; i < 1000; ++i) {
        double randomW = static_cast<double>(std::rand() % 100 + 1);  // Width: 1 to 100
        double randomH = static_cast<double>(std::rand() % 100 + 1); // Height: 1 to 100
        modules.emplace_back(randomW, randomH);
    }

    // Output the generated modules with their W and H coordinates
    std::cout << "Generated 1000 Sample Modules (W, H):\n";
    for (const auto& mod : modules) {
        std::cout << "Module: W = " << mod.width << ", H = " << mod.height << "\n";
    }

    // Perform hMetis partitioning on the modules
    auto partitions = hMetisPartition(modules, 10);  

    // Output the result partitions
    std::cout << "\nPartitions (W, H):\n";
    for (size_t i = 0; i < partitions.size(); ++i) {
        std::cout << "Partition " << i << ":\n";
        for (const auto& mod : partitions[i]) {
            std::cout << "Module: W = " << mod.width << ", H = " << mod.height << "\n";
        }
    }

    // Enumerative packing - generate shape curve for each partition
    std::cout << "\nEnumerative Packing for each partition (W, H):\n";
    std::vector<ShapeCurve> shapeCurves;
    for (size_t i = 0; i < partitions.size(); ++i) {
        std::cout << "Partition " << i << " shape curve (W, H):\n";
        auto curve = enumerativePacking(partitions[i]);
        shapeCurves.push_back(curve);
        printCurve(curve);  // Print the curve (W, H)
    }

    // Merge all shape curves (this step combines the shape curves into one)
    std::cout << "\nMerging All Shape Curves (W, H):\n";
    ShapeCurve mergedCurve;
    for (const auto& curve : shapeCurves) {
        if (mergedCurve.empty()) {
            mergedCurve = curve;
        } else {
            mergedCurve = mergeCurves(mergedCurve, curve);
        }
    }

    // Output the merged curve points (W, H)
    std::cout << "\nMerged Shape Curve (W, H):\n";
    printCurve(mergedCurve);  // Print the merged curve (W, H)

    return 0;
}

// There are still problems with the output, and it is still being improved.