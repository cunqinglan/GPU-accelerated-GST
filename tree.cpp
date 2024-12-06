#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <functional>

// 定义ShapePoint结构体，用于存储形状曲线上的点
struct ShapePoint {
    double width;  // 点的宽度
    double height; // 点的高度
    double area;   // 点的面积，用于排序和比较

    ShapePoint(double w = 0, double h = 0, double a = 0) : width(w), height(h), area(a) {}

    bool operator<(const ShapePoint& other) const {
        return area < other.area;
    }
};

// 定义SlicingTreeNode类型，用于表示Slicing Tree的节点
struct SlicingTreeNode {
    std::vector<SlicingTreeNode*> children; // 子节点
    std::set<ShapePoint> shapeCurve;       // 存储子电路的形状曲线
    bool isHorizontal;    // 是否是水平分割
    bool isSubcircuit;    // 是否是子电路

    SlicingTreeNode() : isHorizontal(false), isSubcircuit(false) {}
};

// 定义ShapeCurve类型，用于存储形状曲线上的点
using ShapeCurve = std::set<ShapePoint>;

// 假设的hMetis分割函数，这里进行递归二分割，直到每个子电路的模块数小于等于maxN
std::vector<std::vector<ShapePoint>> hMetisPartition(const std::vector<ShapePoint>& points, int maxN = 10) {
    std::vector<std::vector<ShapePoint>> partitions;

    // 基本分割条件：当模块数量小于等于maxN时，直接返回
    if (points.size() <= maxN) {
        partitions.push_back(points);
        return partitions;
    }

    // 简化的递归二分割：按宽度分割
    size_t mid = points.size() / 2;
    std::vector<ShapePoint> left(points.begin(), points.begin() + mid);
    std::vector<ShapePoint> right(points.begin() + mid, points.end());

    // 递归分割
    auto leftPartitions = hMetisPartition(left, maxN);
    auto rightPartitions = hMetisPartition(right, maxN);

    // 合并左右分区
    partitions.insert(partitions.end(), leftPartitions.begin(), leftPartitions.end());
    partitions.insert(partitions.end(), rightPartitions.begin(), rightPartitions.end());

    return partitions;
}

// 枚举包装 - 生成所有可能的切割布局
ShapeCurve enumerativePacking(const std::vector<ShapePoint>& points) {
    ShapeCurve result;
    // 使用枚举方式生成所有可能的切割布局
    for (size_t i = 0; i < points.size(); i++) {
        for (size_t j = i+1; j < points.size(); j++) {
            result.insert(ShapePoint(points[i].width + points[j].width, std::max(points[i].height, points[j].height), 0));
        }
    }
    return result;
}

// 水平加法操作：将两个曲线水平方向组合
ShapeCurve addCurvesHorizontally(const ShapeCurve& curveA, const ShapeCurve& curveB) {
    ShapeCurve result;
    for (const auto& pointA : curveA) {
        for (const auto& pointB : curveB) {
            result.insert(ShapePoint(pointA.width + pointB.width, std::max(pointA.height, pointB.height), 0));
        }
    }
    return result;
}

// 翻转操作：基于 W=H 线翻转曲线
ShapeCurve flipCurveVertically(const ShapeCurve& curve) {
    ShapeCurve flippedCurve;
    for (const auto& point : curve) {
        flippedCurve.insert(ShapePoint(point.height, point.width, 0)); // 翻转宽度和高度
    }
    return flippedCurve;
}

// 合并两个曲线，选择较小宽度的点
ShapeCurve mergeCurves(const ShapeCurve& curveA, const ShapeCurve& curveB) {
    ShapeCurve mergedCurve;
    auto itA = curveA.begin();
    auto itB = curveB.begin();

    while (itA != curveA.end() && itB != curveB.end()) {
        const auto& pointA = *itA;
        const auto& pointB = *itB;

        if (pointA.height < pointB.height) {
            mergedCurve.insert(pointA);
            ++itA;
        } else if (pointA.height > pointB.height) {
            mergedCurve.insert(pointB);
            ++itB;
        } else {
            // 在相同高度下，选择宽度较小的点
            mergedCurve.insert(pointA.width < pointB.width ? pointA : pointB);
            ++itA;
            ++itB;
        }
    }

    // 处理剩余部分
    while (itA != curveA.end()) {
        mergedCurve.insert(*itA);
        ++itA;
    }

    while (itB != curveB.end()) {
        mergedCurve.insert(*itB);
        ++itB;
    }

    return mergedCurve;
}

// 合并两个子平面曲线的函数（基于 "⊕" 操作）
ShapeCurve combineShapeCurves(const SlicingTreeNode* node) {
    if (node->children.empty()) {
        return node->shapeCurve; // 如果是叶节点，直接返回该节点的曲线
    }

    // 递归合并子节点的曲线
    ShapeCurve leftCurve = combineShapeCurves(node->children[0]);
    ShapeCurve rightCurve = combineShapeCurves(node->children[1]);

    // 水平加法
    ShapeCurve Ch = addCurvesHorizontally(leftCurve, rightCurve);

    // 翻转
    ShapeCurve Cv = flipCurveVertically(Ch);

    // 合并
    return mergeCurves(Ch, Cv);
}

// 递归构建Slicing Tree的函数
SlicingTreeNode* buildSlicingTree(std::vector<ShapePoint>& points, int maxN = 10) {
    // 使用hMetis进行分区
    auto partitions = hMetisPartition(points, maxN);

    SlicingTreeNode* root = new SlicingTreeNode();

    for (auto& partition : partitions) {
        SlicingTreeNode* childNode = new SlicingTreeNode();
        childNode->shapeCurve = enumerativePacking(partition);
        childNode->isSubcircuit = true;
        root->children.push_back(childNode);
    }

    return root;
}

int main() {
    // 构建Slicing Tree
    std::vector<ShapePoint> points = {
        ShapePoint(1.0, 2.0, 2.0),
        ShapePoint(3.0, 4.0, 12.0),
        ShapePoint(5.0, 6.0, 30.0)
    };
    SlicingTreeNode* tree = buildSlicingTree(points);

    // 合并形状曲线
    ShapeCurve shapeCurve = combineShapeCurves(tree);

    // 输出合并后的形状曲线
    for (const auto& point : shapeCurve) {
        std::cout << "Width: " << point.width << ", Height: " << point.height << ", Area: " << point.area << std::endl;
    }

    // 清理分配的内存
    delete tree;

    return 0;
}
