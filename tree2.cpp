#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <functional>
#include <memory> // for std::unique_ptr

// Define the module structure to store the shape curve modules
struct module {
    double width;  // Width of the point
    double height; // Height of the point
    double area;   // Area of the point used for sorting and comparison

    module(double w = 0, double h = 0, double a = 0) : width(w), height(h), area(a) {}

    bool operator<(const module& other) const {
        if (area == other.area) {
            if (width == other.width) {
                return height < other.height; // Sort by height if width is equal
            }
            return width < other.width; // Sort by width
        }
        return area < other.area; // Sort by area
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
    std::vector<SlicingTreeNode*> children; // Child nodes
    std::vector<module> shapeCurve;        // Store the shape curve of the subcircuit
    bool isHorizontal;                     // Indicates if the division is horizontal
    bool isSubcircuit;                     // Indicates if the node is a subcircuit

    SlicingTreeNode() : isHorizontal(false), isSubcircuit(false) {}
};

// Define ShapeCurve type to store shape curve modules (renamed to avoid conflict)
using ShapeCurve = std::vector<module>;

// Function declarations for the functions we will define later
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

    // Recursively partition the left and right parts
    auto leftPartitions = hMetisPartition(left, maxN);
    auto rightPartitions = hMetisPartition(right, maxN);

    // Merge the left and right partitions
    partitions.insert(partitions.end(), leftPartitions.begin(), leftPartitions.end());
    partitions.insert(partitions.end(), rightPartitions.begin(), rightPartitions.end());

    return partitions;
}

// Enumerative packing - generate all possible cut layouts
ShapeCurve enumerativePacking(const std::vector<module>& modules) {
    std::set<module> resultSet; // Use set to avoid duplicates
    for (size_t i = 0; i < modules.size(); i++) {
        for (size_t j = i + 1; j < modules.size(); j++) {
            double newWidth = modules[i].width + modules[j].width;
            double newHeight = std::max(modules[i].height, modules[j].height);
            double newArea = newWidth * newHeight;  // Compute the area for the combined module
            resultSet.insert(module(newWidth, newHeight, newArea));
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
        combinedCurve = combineShapeCurves(node->children[0]);
    }
    if (node->children.size() > 1) {
        auto rightCurve = combineShapeCurves(node->children[1]);
        combinedCurve = mergeCurves(combinedCurve, rightCurve);
    }

    return combinedCurve;
}

// Horizontal addition operation: combine two curves in the horizontal direction
ShapeCurve addCurvesHorizontally(const ShapeCurve& curveA, const ShapeCurve& curveB) {
    ShapeCurve result;
    for (const auto& pointA : curveA) {
        for (const auto& pointB : curveB) {
            double newWidth = pointA.width + pointB.width;
            double newHeight = std::max(pointA.height, pointB.height);
            result.push_back(module(newWidth, newHeight, newWidth * newHeight));  // Correct area calculation
        }
    }
    return result;
}

// Flip operation: flip the curve along the W=H line
ShapeCurve flipCurveVertically(const ShapeCurve& curve) {
    ShapeCurve flippedCurve;
    for (const auto& point : curve) {
        flippedCurve.push_back(module(point.height, point.width, point.height * point.width)); // Correct area calculation
    }
    return flippedCurve;
}

// Merge two curves, selecting the smaller width point when heights are the same
ShapeCurve mergeCurves(const ShapeCurve& curveA, const ShapeCurve& curveB) {
    ShapeCurve mergedCurve;
    auto itA = curveA.begin();
    auto itB = curveB.begin();

    // Perform the merge based on height, and select the smallest width at each height
    while (itA != curveA.end() && itB != curveB.end()) {
        const auto& pointA = *itA;
        const auto& pointB = *itB;

        if (pointA.height < pointB.height) {
            mergedCurve.push_back(pointA);
            ++itA;
        } else if (pointA.height > pointB.height) {
            mergedCurve.push_back(pointB);
            ++itB;
        } else {
            mergedCurve.push_back(pointA.width < pointB.width ? pointA : pointB);
            ++itA;
            ++itB;
        }
    }

    // Handle remaining parts of the curves
    while (itA != curveA.end()) {
        const auto& pointA = *itA;
        // Add remaining points from curveA with adjusted W-coordinate
        mergedCurve.push_back(module(pointA.width + (itB == curveB.end() ? 0 : curveB.back().width), pointA.height,
                                     (pointA.width + (itB == curveB.end() ? 0 : curveB.back().width)) * pointA.height));
        ++itA;
    }

    while (itB != curveB.end()) {
        const auto& pointB = *itB;
        // Add remaining points from curveB with adjusted W-coordinate
        mergedCurve.push_back(module(pointB.width + (itA == curveA.end() ? 0 : curveA.back().width), pointB.height,
                                     (pointB.width + (itA == curveA.end() ? 0 : curveA.back().width)) * pointB.height));
        ++itB;
    }

    return mergedCurve;
}

// Recursively build the Slicing Tree
std::unique_ptr<SlicingTreeNode> buildSlicingTree(std::vector<module>& modules, int maxN = 10) {
    auto partitions = hMetisPartition(modules, maxN);
    auto root = std::make_unique<SlicingTreeNode>();  // Use smart pointer

    for (auto& partition : partitions) {
        auto childNode = std::make_unique<SlicingTreeNode>();
        childNode->shapeCurve = enumerativePacking(partition);
        childNode->isSubcircuit = true;
        root->children.push_back(std::move(childNode));  // Use move semantics
    }

    return root;
}

int main() {
    // Create a ShapeCurveRange with a specific range
    ShapeCurveRange curveRange(0, 10, 0, 10);

    // Create some sample modules
    std::vector<module> modules = {module(2, 3, 6), module(3, 4, 12), module(5, 6, 30)};
    
    // Perform hMetis partitioning on the modules
    auto partitions = hMetisPartition(modules);

    // Output the result partitions
    for (const auto& partition : partitions) {
        for (const auto& mod : partition) {
            std::cout << "Module: width = " << mod.width << ", height = " << mod.height << ", area = " << mod.area << "\n";
        }
    }

    // Build and display Slicing Tree
    auto slicingTreeRoot = buildSlicingTree(modules);
    
    return 0;
}
