#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <functional>
#include <assert.h>
#include <cmath>
#include <memory> // for std::unique_ptr

using VecCurve = std::vector<double>;
using Node = int;

#pragma region SlicingTreeDef
/* The subcir is with two  parameters. The first indicates min aspect ratio
	 of soft subcir or width of hard subcir. The second indicates max aspect 
	 of soft subcir or height of hard subcir */
struct Subcircuit
{
	Subcircuit() = default;
	Subcircuit(double const& area, bool is_hard, bool is_leaf, double const& par1, double const& par2) : 
		area(area), is_hard(is_hard), is_leaf(is_leaf), par1(par1), par2(par2) {}

	
	double area = -1;
	bool is_hard = false;
	bool is_leaf = false;
	double par1 = -1;
	double par2 = -1;

	VecCurve shapeCurveX;
	VecCurve shapeCurveY;
};

/* In the GST, nodes starts with PI which is smallest subcircuit, then follows
	 internal nodes. Commonly the last node is the root */
struct GST
{
	std::vector<Subcircuit> nodes;

	/* These two vector contains the index of nodes' left and right child. The index
		 of each element indicates index of its parent */
	std::vector<Node> leftChild;
	std::vector<Node> rightChild;

	void createPi(Subcircuit module)
	{
		nodes.push_back(module);
		numPi++;
	}

	int numPi = 0;
};
#pragma endregion

#pragma region SlicingTreeOper
/* function to initialize GST */
void initializeGST()
{
	/* Initialize the GST according to the result of partitioning. 
		 1. Sort the nodes in topological order.
		 2. Build the relationship of nodes and their chilren, child with larger
		 aspect scope is regarded as left. */
}

/* Function to generate points on y = area / x */ 
void generatePoints(Node n, GST& gst, int num_points = 1000) {
	// Calculate the range for x based on the aspect ratio constraints
	std::cout<<"Generating Curve for node "<<n<<"\n";
	auto& node = gst.nodes[n];
	double x_min = std::sqrt(node.area / node.par2);
	double x_max = std::sqrt(node.area/ node.par1);

	double step = (x_max - x_min) / (num_points - 1);

	for (int i = 0; i < num_points; ++i) {
		double x = x_min + i * step; 
		double y = node.area / x;         
		node.shapeCurveX.push_back(x);
		node.shapeCurveY.push_back(y);
	}
}

/* Select the best num nodes with less area */
void getBestN(VecCurve& vecW, VecCurve& vecH, int num)
{
	if (vecW.size() <= num) return;

	VecCurve vecArea;
	int y = 0;
	for (auto& w : vecW)
	{
		double area = w * vecH[y];
		vecArea.push_back(area);
		y++;
	}

	std::vector<double> temp = vecArea;
	std::nth_element(temp.begin(), temp.begin() + num, temp.end());
	double nthValue = temp[num];

	VecCurve BestW;
	VecCurve BestH;
	int idx = 0;
	for (auto const& area : vecArea)
	{
		if (area <= nthValue)
		{
			BestW.push_back(vecW[idx]);
			BestH.push_back(vecH[idx]);
		}
    idx++;
	}
	vecW = std::move(BestW);
	vecH = std::move(BestH);
}

/* Flip the curve and save the best 1000 nodes */
void flipCurve(Node n, GST& gst)
{
	VecCurve newCurveX;
	VecCurve newCurveY;
	auto& originalCurveX = gst.nodes[n].shapeCurveX;
	auto& originalCurveY = gst.nodes[n].shapeCurveY;

	int i = 0, j = originalCurveY.size() - 1;

	while (i < originalCurveX.size() && j >= 0)
	{
		if (originalCurveX[i] > originalCurveY[j])
		{
			newCurveX.push_back(originalCurveY[j]);
			newCurveY.push_back(originalCurveX[j]);
			j--;
		}
		else
		{
			newCurveX.push_back(originalCurveX[i]);
			newCurveY.push_back(originalCurveY[i]);
			i++;
		}
	}

	while (i < originalCurveX.size())
	{
		newCurveX.push_back(originalCurveX[i]);
		newCurveY.push_back(originalCurveY[i]);
		i++;
	}
	while (j >= 0)
	{
		newCurveX.push_back(originalCurveY[j]);
		newCurveY.push_back(originalCurveX[j]);
		j--;
	}

	/* Save the best 1000 nodes with less area */
	getBestN(newCurveX, newCurveY, 1000);

	gst.nodes[n].shapeCurveX = std::move(newCurveX);
	gst.nodes[n].shapeCurveY = std::move(newCurveY);
}

/* Combine Curves of children of given node. This function can only be applied
	 on internal sub-partitions */
void combineNode(Node n, GST& gst)
{
	std::cout<<"Combining "<<n<<", "<<"merging Curve of node "<<gst.leftChild[n]<<" and "<<gst.rightChild[n]<<"\n";
	auto const& left = gst.nodes[gst.leftChild[n]];
  auto const& right = gst.nodes[gst.rightChild[n]];
	auto& node = gst.nodes[n];
	std::cout<<"sizeLeftChild = "<<left.shapeCurveX.size()<<"\t"<<"sizeRightChild = "<<right.shapeCurveX.size()<<"\n";

	double epsilon = 1e-5;

	/* Check if there has been curve in child */
	if (left.shapeCurveX.empty() || right.shapeCurveX.empty())
	{
		std::cerr<<"Error when dealing node "<<n<<"\n";
	}
	assert(!(left.shapeCurveX.empty() || right.shapeCurveX.empty()) && "Curve of child is not computed yet");

	int ri = 0;
	int li = 0;
	int rsize = right.shapeCurveX.size();
	for (auto const& ly : left.shapeCurveY)
	{
		if (ly >= right.shapeCurveY[ri] - epsilon) {
			node.shapeCurveX.push_back(left.shapeCurveX[li] + right.shapeCurveX[ri]);
			node.shapeCurveY.push_back(ly);
		}
		else {
			ri++;
			node.shapeCurveX.push_back(left.shapeCurveX[li] + right.shapeCurveX[ri]);
			node.shapeCurveY.push_back(ly);
			/* If there is no element in the right child, end the iteration*/
			if (ri > rsize) break;
		}
    li++;
	}

	flipCurve(n, gst);
	std::cout<<"sizeResultCurveSize = "<<node.shapeCurveX.size()<<"\n";
}

GST fakePartition()
{
	GST gst;
	for (int i = 0; i < 7; i++)
	{
		gst.createPi({10, false, true, 0.1, 10});
	}
	for (int i = 0; i < 6; i++)
	{
		gst.nodes.emplace_back();
	}
	for (int i = 0; i < 7; i++)
	{
		gst.leftChild.push_back(-1);
		gst.rightChild.push_back(-1);
	}
	gst.leftChild.push_back(0);
	gst.rightChild.push_back(1);
	gst.leftChild.push_back(2);
	gst.rightChild.push_back(3);
	gst.leftChild.push_back(4);
	gst.rightChild.push_back(5);
	gst.leftChild.push_back(7);
	gst.rightChild.push_back(8);
	gst.leftChild.push_back(9);
	gst.rightChild.push_back(6);
	gst.leftChild.push_back(10);
	gst.rightChild.push_back(11);
	
	return gst;
}

/* Print all coordinates of given node */
void printCurve(Node n, GST& gst)
{
	auto const& node = gst.nodes[n];
	int y = 0;
	for (auto x : node.shapeCurveX)
	{
		std::cout<<"x = "<<x<<", "<<"y = "<<node.shapeCurveY[y]<<"\n";
		y++;
	}
}
#pragma endregion

#pragma region foreach
template<typename Fn>
void foreach_pi(GST& gst, Fn&& fn)
{
	int idx = 0;
	for (auto& node : gst.nodes)
	{
		if (idx >= gst.numPi) return;
		else {
			fn(idx);
			idx++;
		}
	}
}

template<typename Fn>
void foreach_node(GST& gst, Fn&& fn)
{
	int idx = 0;
	for (auto& node : gst.nodes)
	{		
		fn(idx);
		idx++;
	}
}

template<typename Fn>
void foreach_partition(GST& gst, Fn&& fn)
{
	int idx = 0;
	for (auto& node : gst.nodes)
	{		
		if (idx < gst.numPi)
		{
			idx++;
			continue;
		}
		fn(idx);
		idx++;
	}
}
#pragma endregion

int main() {
	GST gst = fakePartition();
	auto& node = gst.nodes[0];
	int y = 0;
	// for (auto x : node.shapeCurveX)
	// {
	// 	std::cout<<"x = "<<x<<", y = "<<node.shapeCurveY[y]<<"\n";
	// 	y++;
	// }
	for (int i = 0; i < 7; i++)
  {
		generatePoints(i, gst);
	}
	combineNode(7, gst);
	// foreach_partition(gst, [&](Node& node){
	// 	combineNode(node, gst);
	// });
  // std::cout<<gst.numPi;
	printCurve(7, gst);
}
