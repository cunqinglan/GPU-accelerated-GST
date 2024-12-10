#include <cmath>
#include "GSTrevise.hpp"
#include <stdio.h>
#include <chrono>


__global__ void cudaGenerateCurve(float* dCurveX, float* dCurveY, float area, float start, float interval, int size)
{
  int id = blockIdx.x * blockDim.x + threadIdx.x;
  float x = start + id * interval;
  if (id < size)
  {
    dCurveX[id] = x;
    dCurveY[id] = area / x;
  }
}

void GPUgenerateCurve(Node n, GST& gst, float* dCurveX, float* dCurveY, int blockSize = 512)
{
  auto& nodeData = gst.nodes[n];
  std::cout << "curve size = " << nodeData.shapeCurveX.size() << "\n";

  size_t arraySize = 1048576;
  int iBytes = arraySize * sizeof(float);

  int gridSize = (arraySize + blockSize - 1) / blockSize;

  dim3 block(blockSize);
  dim3 grid(gridSize);

  float maxX = sqrt(nodeData.area / nodeData.par1);
  float minX = sqrt(nodeData.area / nodeData.par2);

  float interval = (maxX - minX) / arraySize;

  auto startKernel = std::chrono::high_resolution_clock::now();
  cudaGenerateCurve<<<grid, block>>>(dCurveX, dCurveY, nodeData.area, minX, interval, arraySize);
  cudaDeviceSynchronize();
  auto endKernel = std::chrono::high_resolution_clock::now();

  std::vector<float> hCurveX(arraySize);
  std::vector<float> hCurveY(arraySize);

  auto startMemcpy = std::chrono::high_resolution_clock::now();
  cudaMemcpy(hCurveX.data(), dCurveX, arraySize * sizeof(float), cudaMemcpyDeviceToHost);
  cudaMemcpy(hCurveY.data(), dCurveY, arraySize * sizeof(float), cudaMemcpyDeviceToHost);
  auto endMemcpy = std::chrono::high_resolution_clock::now();

  auto startMove = std::chrono::high_resolution_clock::now();
  nodeData.shapeCurveX = std::move(hCurveX);
  nodeData.shapeCurveY = std::move(hCurveY);
  auto endMove = std::chrono::high_resolution_clock::now();


  std::cout << "Kernel execution took: " 
            << std::chrono::duration_cast<std::chrono::nanoseconds>(endKernel - startKernel).count() << " ns\n";
  std::cout << "Memory copy took: " 
            << std::chrono::duration_cast<std::chrono::nanoseconds>(endMemcpy - startMemcpy).count() << " ns\n";
  std::cout << "Move operations took: " 
            << std::chrono::duration_cast<std::chrono::nanoseconds>(endMove - startMove).count() << " ns\n";
}


// int main()
// {
//   size_t arraySize = 512;
//   std::vector<std::vector<float>> shapeX(3, std::vector<float>(512, 0));
//   std::vector<std::vector<float>> shapeY(3, std::vector<float>(512, 0));
  
//   std::vector<Subcircuit> functionList(3);
//   functionList[0] = Subcircuit(10, false, true, 0.3, 3);
//   functionList[1] = Subcircuit(20, false, true, 0.3, 3);
//   functionList[2] = Subcircuit(30, false, true, 0.2, 5);

//   for (auto& i : functionList)
//   {
//     cpuGenerateCurve(i);
//   }
// }



void measureExecutionTime(const std::string& label, const std::function<void()>& func) {
    auto start = std::chrono::high_resolution_clock::now();

    func();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << label << " took " << duration << " ms\n";
}

int main() {
  GST gst = fakePartition();

  size_t arraySize = 1048576;
  int iBytes = arraySize * sizeof(float);

  float *dCurveX, *dCurveY;

  auto startMalloc = std::chrono::high_resolution_clock::now();
  cudaMalloc((float**)&dCurveX, iBytes);
  cudaMalloc((float**)&dCurveY, iBytes);
  auto endMalloc = std::chrono::high_resolution_clock::now();

  cudaMemset(dCurveX, 0, iBytes);
  cudaMemset(dCurveY, 0, iBytes);

  std::cout << "Memory allocation took: " 
            << std::chrono::duration_cast<std::chrono::nanoseconds>(endMalloc - startMalloc).count() << " ns\n";


  std::cout << "Measuring CPU function:\n";
  measureExecutionTime("CPU Function", [&]() {

  generatePoints(1, gst);
  });

  std::cout << "Measuring GPU function:\n";
  measureExecutionTime("GPU Function", [&]() {
    GPUgenerateCurve(2, gst, dCurveX, dCurveY);
  });
  std::cout << "Measuring GPU function:\n";
  measureExecutionTime("GPU Function", [&]() {
    GPUgenerateCurve(3, gst, dCurveX, dCurveY);
  });

  return 0;
}