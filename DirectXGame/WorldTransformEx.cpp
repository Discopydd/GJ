#include<KamataEngine.h>

using namespace KamataEngine;
using namespace MathUtility;

void WorldTransform::UpdateMatrix() {
	Matrix4x4 matScale = MakeScaleMatrix(scale_);

	Matrix4x4 matRotX = MakeRotateXMatrix(rotation_.x);
	Matrix4x4 matRotY = MakeRotateXMatrix(rotation_.y);
	Matrix4x4 matRotZ = MakeRotateXMatrix(rotation_.z);
	Matrix4x4 matRot = matRotZ * matRotX * matRotY;

	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);
	matWorld_ = matScale * matRot * matTrans;

	TransferMatrix();
}