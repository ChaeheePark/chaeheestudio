#pragma warning (disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <math.h>

// 영상 화소 값 반전
void InverseImage(BYTE* Img, BYTE* Out, int W, int H) {
	int ImgSize = W * H;
	for (int i = 0; i < ImgSize; i++) {
		Out[i] = 255 - Img[i];
	}
}

void SaveBMPFile(BITMAPFILEHEADER hf, BITMAPINFOHEADER hInfo, RGBQUAD* hRGB,
	BYTE* Out, int W, int H, const char* FileName) {

	FILE* fp = fopen(FileName, "wb");
	fwrite(&hf, sizeof(BYTE), sizeof(BITMAPFILEHEADER), fp); //binary file은 byte 단위로 의미가 있음, 그래서 읽어올때와 다르게 쓸 때는 byte 단위로 써야 더 의미있음
	fwrite(&hInfo, sizeof(BYTE), sizeof(BITMAPINFOHEADER), fp);
	fwrite(hRGB, sizeof(RGBQUAD), 256, fp);
	fwrite(Out, sizeof(BYTE), W * H, fp);
	fclose(fp);

}
// 영상 밝기 조절
void BrightnessAdj(BYTE* Img, BYTE* Out, int W, int H, int Val) {
	int ImgSize = W * H;
	for (int i = 0; i < ImgSize; i++) {
		if (Img[i] + Val > 255) Out[i] = 255;
		else if (Img[i] + Val < 0) Out[i] = 0;
		else Out[i] = Img[i] + Val;
	}
}

// 영상 대비 조절
void ContrastAdj(BYTE* Img, BYTE* Out, int W, int H, double Val) {
	int ImgSize = W * H;
	for (int i = 0; i < ImgSize; i++) {
		if (Img[i] * Val > 255.0) Out[i] = 255;
		else Out[i] = (BYTE)(Img[i] * Val);
	}
}

// 화소 Histogram
void ObtainHistogram(BYTE* Img, int* Histo, int W, int H) {
	int ImgSize = W * H;
	for (int i = 0; i < ImgSize; i++)  Histo[Img[i]]++;
}

// Histo의 누적 Histogram
void ObtainAHistogram(int* Histo, int* AHisto) {
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j <= i; j++) {
			AHisto[i] += Histo[j];
		}
	}
}

// 스트레칭
void HistogramStretching(BYTE* Img, BYTE* Out, int* Histo, int W, int H) {
	int ImgSize = W * H;
	BYTE Low, High;
	for (int i = 0; i < 255; i++) {
		if (Histo[i] != 0) {
			Low = i;
			break;
		}
	}
	for (int i = 255; i >= 0; i--) {
		if (Histo[i] != 0) {
			High = i;
			break;
		}
	}

	for (int i = 0; i < ImgSize; i++) {
		Out[i] = (BYTE)(Img[i] - Low) / (double)(High - Low) * 255.0;
	}

}

// 평활화
void HistogramEqualization(BYTE* Img, BYTE* Out, int* AHisto, int W, int H) {
	int ImgSize = W * H;
	int Nt = ImgSize;
	int Gmax = 255;
	double Ratio = Gmax / (double)Nt;
	BYTE NormSum[256];
	for (int i = 0; i < 256; i++) {
		NormSum[i] = (BYTE)(Ratio * AHisto[i]);
	}
	for (int i = 0; i < ImgSize; i++) {
		Out[i] = NormSum[Img[i]];
	}
}

// 곤잘레스 임계값 정하기
BYTE DetermThGonzalez(int* Histo) {
	int ep = 3;
	BYTE Low, High;
	BYTE Th, NewTh;
	int G1 = 0, G2 = 0, cnt1 = 0, cnt2 = 0, mu1, mu2;
	for (int i = 0; i < 255; i++) {
		if (Histo[i] != 0) {
			Low = i;
			break;
		}
	}
	for (int i = 255; i >= 0; i--) {
		if (Histo[i] != 0) {
			High = i;
			break;
		}
	}
	Th = (Low + High) / 2; //초기 임계값
	while (1) {
		for (int i = Th + 1; i <= High; i++) {
			G1 += Histo[i] * i;
			cnt1 += Histo[i];
		}
		for (int i = Low; i <= Th; i++) {
			G2 += Histo[i] * i;
			cnt2 += Histo[i];
		}

		if (cnt1 == 0) cnt1 = 1;
		if (cnt2 == 0) cnt2 = 1;
		mu1 = G1 / cnt1;
		mu2 = G2 / cnt2;
		NewTh = (mu1 + mu2) / 2;

		if (abs(NewTh - Th) < ep) {
			Th = NewTh;
			return Th;
		}
		else Th = NewTh;
		G1 = G2 = cnt1 = cnt2 = 0;
	}
}

// 영상 이진화 (임계값을 기준으로)
void Binarization(BYTE* Img, BYTE* Out, int W, int H, BYTE Threshold) {
	int ImgSize = W * H;
	for (int i = 0; i < ImgSize; i++) {
		if (Img[i] < Threshold) Out[i] = 0;
		else Out[i] = 255;
	}
}

// average lowpass(영상흐려짐)
void AverageConv(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { 0.11111,0.11111,0.11111,
							0.11111,0.11111,0.11111,
							0.11111,0.11111,0.11111 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			Out[i * W + j] = (BYTE)SumProduct;
			SumProduct = 0;
		}
	}
}


// 가우시안평활화
void GaussAvgConv(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { 0.0625,0.125,0.0625,
							0.125,0.25,0.125,
							0.0625,0.125,0.0625 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			Out[i * W + j] = (BYTE)SumProduct;
			SumProduct = 0;
		}
	}
}


// Prewitt 마스크
void Prewitt_X_Conv(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { -1.0,0.0,1.0,
							-1.0,0.0,1.0,
							-1.0,0.0,1.0 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			// 절대값 씌우면 0~765 사이의 값이 나올거임
			Out[i * W + j] = abs((long)SumProduct) / 3;
			SumProduct = 0;
		}
	}
}

void Prewitt_Y_Conv(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { -1.0,-1.0,-1.0,
							0.0,0.0,0.0,
							1.0,1.0,1.0 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			// 절대값 씌우면 0~765 사이의 값이 나올거임
			Out[i * W + j] = abs((long)SumProduct) / 3;
			SumProduct = 0;
		}
	}
}

// Sobel 마스크
void Sobel_X_Conv(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { -1.0,0.0,1.0,
							-2.0,0.0,2.0,
							-1.0,0.0,1.0 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			// 절대값 씌우면 0~765 사이의 값이 나올거임
			Out[i * W + j] = abs((long)SumProduct) / 4;
			SumProduct = 0;
		}
	}
}

void Sobel_Y_Conv(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { -1.0,-2.0,-1.0,
							0.0,0.0,0.0,
							1.0,2.0,1.0 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			// 절대값 씌우면 0~765 사이의 값이 나올거임
			Out[i * W + j] = abs((long)SumProduct) / 4;
			SumProduct = 0;
		}
	}
}

//Laplace 마스크
void Laplace_Conv(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { -1.0,-1.0,-1.0,
							-1.0,8.0,-1.0,
							-1.0,-1.0,-1.0 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			// 절대값 씌우면 0~765 사이의 값이 나올거임
			Out[i * W + j] = abs((long)SumProduct) / 8;
			SumProduct = 0;
		}
	}
}

void Laplace_Conv_DC(BYTE* Img, BYTE* Out, int W, int H) {
	double Kernel[3][3] = { -1.0,-1.0,-1.0,
							-1.0,9.0,-1.0,
							-1.0,-1.0,-1.0 };
	double SumProduct = 0.0;
	for (int i = 1; i < H - 1; i++) { //margin을 주기위해 1부터 시작 ~ H-1 까지
		for (int j = 1; j < W - 1; j++) {
			for (int m = -1; m <= 1; m++) { // 3*3 kernel 에서 Height 
				for (int n = -1; n <= 1; n++) { //  Width 나타내기위해
					SumProduct += Img[(i + m) * W + (j + n)] * Kernel[m + 1][n + 1]; //Y*W+X
				}
			}
			//원래 영상의 밝기 유지
			if (SumProduct > 255.0) Out[i * W + j] = 255;
			else if (SumProduct < 0.0) Out[i * W + j] = 0;
			else Out[i * W + j] = (BYTE)SumProduct;
			SumProduct = 0.0;
		}
	}
}

void swap(BYTE* a, BYTE* b) {
	BYTE tp = *a;
	*a = *b;
	*b = tp;
}

// 9개 화소값중 가운데 값으로 치환 (솔트페퍼 노이즈 감소)
BYTE Median(BYTE* arr, int size) {
	const int S = size;
	for (int i = 0; i < size - 1; i++) {//pivot index
		for (int j = 0; j < size; j++) {// 비교대상 index
			if (arr[i] > arr[j]) swap(&arr[i], &arr[j]);
		}
	}
	return arr[size / 2];
}

//9개 화소값중 가장 큰 값으로 치환
BYTE MaxPooling(BYTE* arr, int size) {
	const int S = size;
	for (int i = 0; i < size - 1; i++) {//pivot index
		for (int j = 0; j < size; j++) {// 비교대상 index
			if (arr[i] > arr[j]) swap(&arr[i], &arr[j]);
		}
	}
	return arr[S - 1];
}

//9개 화소값중 가장 작은 값으로 치환 솔트나 페퍼노이즈중 하나만 영상에서 많이 존재할 때 사용
BYTE MinPooling(BYTE* arr, int size) {
	const int S = size;
	for (int i = 0; i < size - 1; i++) {//pivot index
		for (int j = 0; j < size; j++) {// 비교대상 index
			if (arr[i] > arr[j]) swap(&arr[i], &arr[j]);
		}
	}
	return arr[0];
}


//라벨링
int push(short* stackx, short* stacky, int arr_size, short vx, short vy, int* top)
{
	if (*top >= arr_size) return(-1);
	(*top)++;
	stackx[*top] = vx;
	stacky[*top] = vy;
	return(1);
}

int pop(short* stackx, short* stacky, short* vx, short* vy, int* top)
{
	if (*top == 0) return(-1);
	*vx = stackx[*top];
	*vy = stacky[*top];
	(*top)--;
	return(1);
}


// GlassFire 알고리즘을 이용한 라벨링 함수
void m_BlobColoring(BYTE* CutImage, int height, int width)
{
	int i, j, m, n, top, area, Out_Area, index, BlobArea[1000];
	long k;
	short curColor = 0, r, c;
	//	BYTE** CutImage2;
	Out_Area = 1;


	// 스택으로 사용할 메모리 할당
	short* stackx = new short[height * width];
	short* stacky = new short[height * width];
	short* coloring = new short[height * width];

	int arr_size = height * width;

	// 라벨링된 픽셀을 저장하기 위해 메모리 할당

	for (k = 0; k < height * width; k++) coloring[k] = 0;  // 메모리 초기화

	for (i = 0; i < height; i++)
	{
		index = i * width;
		for (j = 0; j < width; j++)
		{
			// 이미 방문한 점이거나 픽셀값이 255가 아니라면 처리 안함
			if (coloring[index + j] != 0 || CutImage[index + j] != 255) continue;
			r = i; c = j; top = 0; area = 1;
			curColor++;

			while (1)
			{
			GRASSFIRE:
				for (m = r - 1; m <= r + 1; m++)
				{
					index = m * width;
					for (n = c - 1; n <= c + 1; n++)
					{
						//관심 픽셀이 영상경계를 벗어나면 처리 안함
						if (m < 0 || m >= height || n < 0 || n >= width) continue;

						if ((int)CutImage[index + n] == 255 && coloring[index + n] == 0)
						{
							coloring[index + n] = curColor; // 현재 라벨로 마크
							if (push(stackx, stacky, arr_size, (short)m, (short)n, &top) == -1) continue;
							r = m; c = n; area++;
							goto GRASSFIRE;
						}
					}
				}
				if (pop(stackx, stacky, &r, &c, &top) == -1) break;
			}
			if (curColor < 1000) BlobArea[curColor] = area;
		}
	}

	float grayGap = 255.0f / (float)curColor;
	//curColor 25개 (threshold 로 영역을 나눈것.)
	//grayGap 얼마나 밝기 단위할것인가 (10.2 간격정도로) 10.2 -> 20.4 등등
	//blobArea는 1부터 특정 영역이 몇 pixel의 면적을 가지는지에 대한 배열 component의 크기

	// 가장 면적이 넓은 영역을 찾아내기 위함 
	for (i = 1; i <= curColor; i++)
	{
		if (BlobArea[i] >= BlobArea[Out_Area]) Out_Area = i;
	}
	// CutImage 배열 클리어~
	for (k = 0; k < width * height; k++) CutImage[k] = 255;

	//CutImage는 Output배열, 255로 초기화시킴 하얗게 채움

	// coloring에 저장된 라벨링 결과중 (Out_Area에 저장된) 영역이 가장 큰 것만 CutImage에 저장
	for (k = 0; k < width * height; k++)
	{
		if (coloring[k] == Out_Area) CutImage[k] = 0;  // 가장 큰 것만 저장(size filtering)
		//CutImage[k] = (unsigned char)(coloring[k] * grayGap);
		//coloring 배열은 몇변영역인지 (1~25를 반환)
		//if (BlobArea[coloring[k]] > 500) CutImage[k] = 0; //500 이상 영역만 filtering
	}

	delete[] coloring;
	delete[] stackx;
	delete[] stacky;
}
// 라벨링 후 가장 넓은 영역에 대해서만 뽑아내는 코드 포함

void BinaryImageEdgeDetetion(BYTE* Bin, BYTE* Out, int W, int H) {
	//가장 큰 면적의 경계출력코드
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			if (Bin[i * W + j] == 0) {//!(전경화소라면), 4방향 화소중 하나라도 전경이 아니라면
				if (!(Bin[(i - 1) * W + j] == 0 &&
					Bin[(i + 1) * W + j] == 0 &&
					Bin[i * W + j - 1] == 0 && Bin[i * W + j + 1] == 0)) {
					Out[i * W + j] = 255;
				}

			}
		}
	}
}

void DrawRectOutline(BYTE* Img, int W, int H, int LU_X, int LU_Y, int RD_X, int RD_Y) {
	// Img: 사각형을 그릴 이미지배열, W: 영상 가로사이즈, H: 영상 세로사이즈,
	// LU_X: 사각형의 좌측상단 X좌표, LU_Y: 사각형의 좌측상단 Y좌표,
	// RD_X: 사각형의 우측하단 X좌표, LU_Y: 사각형의 우측하단 Y좌표.
	for (int j = LU_X; j <= RD_X; j++) {
		Img[LU_Y * W + j] = 255;
	}
	for (int j = LU_X; j <= RD_X; j++) {
		Img[RD_Y * W + j] = 255;
	}
	for (int i = LU_Y; i <= RD_Y; i++) {
		Img[i * W + LU_X] = 255;
	}
	for (int i = LU_Y; i <= RD_Y; i++) {
		Img[i * W + RD_X] = 255;
	}
}

void DrawCrossLine(BYTE* Img, int W, int H, int Cx, int Cy) {
	// Img: 가로/세로 라인을 그릴 이미지배열, W: 영상 가로사이즈, H: 영상 세로사이즈,
	// Cx: 가로/세로 라인이 교차되는 지점의 X좌표
	// Cy: 가로/세로 라인이 교차되는 지점의 Y좌표
	for (int i = 0; i <= W; i++) {
		Img[Cy * W + i] = 255;
	}
	for (int j = 0; j <= H; j++) {
		Img[j * W + Cx] = 255;
	}
}

//영상 위 아래를 바꿈
void VerticalFlip(BYTE* Img, int W, int H) {
	for (int i = 0; i < H / 2; i++) {
		for (int j = 0; j < W; j++) {
			swap(&Img[i * W + j], &Img[(H - 1-i) * W + j]);
		}
	}
}

//영상 왼쪽 오른쪽을 바꿈
void HorizontalFlip(BYTE* Img, int W, int H) {
	for (int i = 0; i < W / 2; i++) {
		for (int j = 0; j < H; j++) {
			swap(&Img[j * W + i], &Img[ j * W + (W-1-i)]);
		}
	}
}

// 영상 이동 근데 y에 -1곱해주는 이유는 bmp파일이 애초에 위 아래 바뀌어져있어서 내가 원하는 방향을 얻으려면 -1 해줘야함
void Translation(BYTE* Img, BYTE* Out, int W, int H, int Tx, int Ty) {
	Ty *= -1;
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			if ((i + Ty < H && i + Ty >= 0) && (j + Tx < W && j + Tx >= 0)) Out[(i + Ty) * W + (j + Tx)] = Img[i * W + j];
		}
	}
}

//영상 축소, 확대
void Scaling(BYTE* Img, BYTE* Out, int W, int H, double SF_X, double SF_Y) {
	//SF x,y 1보다 크면 확대 1보다 작으면 축소
	int tmpX, tmpY;
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			tmpX = (int)(j / SF_X), tmpY = (int)(i / SF_Y); //원래는 곱해줘야하는데 역방향사상이라서 나눠줌
			if (tmpY < H && tmpX < W) { //순방향사상하면 hole 이생김, 그래서 역방향사상을 진행
				Out[i * W + j] = Img[tmpY * W + tmpX];
			}
		}
	}
}

// 영상 회전
void Rotation(BYTE* Img, BYTE* Out, int W, int H, int Angle) {
	int tmpX, tmpY;
	double Radian = Angle * 3.141592 / 180.0;
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			tmpX = (int)(cos(Radian) * j + sin(Radian) * i);
			tmpY = (int)(-sin(Radian) * j + cos(Radian) * i); //역행렬로 구성
			if ((tmpY < H && tmpY>=0) && (tmpX < W && tmpX>=0)) { //순방향사상하면 hole 이생김, 그래서 역방향사상을 진행
				Out[i * W + j] = Img[tmpY * W + tmpX];
			}
		}
	}
}


int main() {

	//영상입력
	BITMAPFILEHEADER hf;
	BITMAPINFOHEADER hInfo;
	RGBQUAD hRGB[256];
	FILE* fp;

	fp = fopen("lenna.bmp", "rb");
	if (fp == NULL) {
		printf("file not found!\n");
		return -1;
	}

	fread(&hf, sizeof(BITMAPFILEHEADER), 1, fp); //14byte를 1번 읽어와서 hf에 저장해라
	fread(&hInfo, sizeof(BITMAPINFOHEADER), 1, fp);
	fread(hRGB, sizeof(RGBQUAD), 256, fp);
	int ImgSize = hInfo.biWidth * hInfo.biHeight;

	BYTE* Image = (BYTE*)malloc(ImgSize);
	BYTE* Output = (BYTE*)malloc(ImgSize);
	BYTE* Temp = (BYTE*)malloc(ImgSize);
	fread(Image, sizeof(BYTE), ImgSize, fp);
	fclose(fp);

	int W = hInfo.biWidth, H = hInfo.biHeight;
	int Histo[256] = { 0 };
	int AHisto[256] = { 0 };
	//영상처리

	/*InverseImage(Image, Output, W, H);
	BrightnessAdj(Image, Output, W, H, 70);
	ContrastAdj(Image, Output, W, H, 1.5);*/

	// 히스토그램
	ObtainHistogram(Image, Histo, W, H);
	// 누적 히스토그램
	ObtainAHistogram(Histo, AHisto);
	
	//VerticalFlip(Image, W, H);
	//HorizontalFlip(Image, W, H);
	//Translation(Image, Output, W, H, 100, 40);
	//Scaling(Image, Output, W, H, 0.7, 0.7); //Uniform scaling 
	Rotation(Image, Output, W, H, 30);


	//평활화
	//AverageConv(Image, Output, W, H);
	//GaussAvgConv(Image, Output, W, H);

	////경계검출 후 이진화
	//Prewitt_X_Conv(Image, Temp, W, H);
	//Prewitt_Y_Conv(Image, Output, W, H);
	//for (int i = 0; i < ImgSize; i++) {
	//	if (Temp[i] > Output[i]) Output[i] = Temp[i];
	//}
	//Binarization(Output, Output, W, H, 50);

	
	//Sobel_X_Conv(Image, Temp, W, H);
	//Sobel_Y_Conv(Image, Output, W, H);
	//for (int i = 0; i < ImgSize; i++) {
	//	if (Temp[i] > Output[i]) Output[i] = Temp[i];
	//}
	//Binarization(Output, Output, W, H, 50);

	//평활화 후 경계검출
	//Laplace_Conv(Image, Output, W, H);
	//GaussAvgConv(Image, Output, W, H);
	//Laplace_Conv_DC(Image, Output, W, H);



	//스트레칭
	//HistogramStretching(Image, Output, Histo, W, H);

	//평활화
	//HistogramEqualization(Image, Output, AHisto, W, H);

	/*이진화
	BYTE Th;
	Th = DetermThGonzalez(Histo);
	printf("%d", Th);
	Binarization(Image, Output, W, H, Th);*/

	/*median filtering
	BYTE temp[9];
	int i, j;
	for (i = 1; i < H - 1; i++) {
		for (j = 0; j < W - 1; j++) {
			temp[0] = Image[(i - 1) * W + j-1];
			temp[1] = Image[(i-1) * W + j];
			temp[2] = Image[(i - 1) * W + j+1];
			temp[3] = Image[i * W + j-1];
			temp[4] = Image[i * W + j];
			temp[5] = Image[i * W + j+1];
			temp[6] = Image[(i + 1)* W + j-1];
			temp[7] = Image[(i + 1 )* W + j];
			temp[8] = Image[(i + 1 )* W + j+1];
			Output[i * W + j] = Median(temp, 9);
			//Output[i * W + j] = MaxPooling(temp, 9);
			//Output[i * W + j] = MinPooling(temp, 9);
		}
	}*/

	///* median filtering*/
	//int length = 3;  // 마스크의 한 변의 길이
	//int margin = length / 2;
	//int wsize = length * length;
	//byte* temp = (byte*)malloc(sizeof(byte) * wsize);

	//int i, j, m, n;
	//for (i = margin; i < h - margin; i++) {
	//	for (j = margin; j < w - margin; j++) {
	//		for (m = -margin; m <= margin; m++) {
	//			for (n = -margin; n <= margin; n++) {
	//				temp[(m + margin) * length + (n + margin)] = image[(i + m) * w + j + n];
	//			}
	//		}
	//		temp[i * w + j] = median(temp, wsize);
	//	}
	//}
	//free(temp);
	//

	//가장큰 영역검출
	//Binarization(Image, Temp, W, H, 50);
	//InverseImage(Temp, Temp, W, H);
	//m_BlobColoring(Temp, H, W);
	//for (int i = 0; i < ImgSize; i++) Output[i] = Image[i];
	//BinaryImageEdgeDetetion(Temp, Output, W, H);

	//int y1 = 0, y2 = 0, x1 = 0, x2 = 0;
	////x1, x2, y1, y2 구하기
	//for (int i = 0; i < ImgSize; i++) {
	//	if (Temp[i] == 0) {
	//		y1 = (int)i / W;
	//		break;
	//	}
	//}
	//for (int i = ImgSize; i >= 0; i--) {
	//	if (Temp[i] == 0) {
	//		y2 = (int)i / W;
	//		break;
	//	}
	//}
	//for (int i = 0; i < ImgSize; i++) {
	//	if (Temp[(i * W) % (ImgSize - 1)] == 0) {
	//		x1 = (i * W) % (ImgSize - 1) % W;
	//		break;
	//	}
	//}
	//for (int i = ImgSize; i >= 0; i--) {
	//	if (Temp[(i * W) % (ImgSize - 1)] == 0) {
	//		x2 = (i * W) % (ImgSize - 1) % W;
	//		break;
	//	}
	//}
	////printf("%d, %d, %d,  %d\n",x1,x2,y1,y2);
	//DrawRectOutline(Output, W, H, x1, y1, x2, y2);

	////cx, cy구하기
	//int sumx = 0, sumy = 0, count = 0;
	//for (int i = x1; i <= x2; i++) {
	//	for (int j = y1; j <= y2; j++) {
	//		if (Temp[j * W + i] == 0) {
	//			sumx += i;
	//			sumy += j;
	//			count++;
	//		}
	//	}
	//}

	////printf("%d,%d\n", sumx, sumy);
	//int cx = (int)sumx / count;
	//int cy = (int)sumy / count;
	////printf("%d, %d", cx, cy);
	//DrawCrossLine(Output, W, H, cx, cy);


	//결과출력
	SaveBMPFile(hf, hInfo, hRGB, Output, W, H, "outputt.bmp");
	free(Image);
	free(Output);
	free(Temp);
	return 0;
}