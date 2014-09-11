#ifndef __KDTREE_H__
#define __KGTREE_H__
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include "kgmacro.h"
#include "kgstdint.h"
#include "types.h"


//----------------------------------------------------------------
// KD-treeクラス
//----------------------------------------------------------------
template<typename T>
class KDTree {
public:
	// k-NN searchのクエリ（質問）
	struct Query {
		float max_distance2; // 探索の最大半径
		size_t max_search_num; // 最大探索点数
		Vec search_position; // 探索中心
		Vec normal; // 探索中心における法線
		Query(const Vec &search_position_, const Vec &normal_, const float max_distance2_, const size_t max_search_num_) :
			max_distance2(max_distance2_), 
			normal(normal_), 
			max_search_num(max_search_num_), 
			search_position(search_position_) {}
	};
	// 結果のQueueに乗せるためのデータ構造。
	struct ElementForQueue {
		const T *point;
		float distance2;
		ElementForQueue(const T *point_, const float distance2_) : point(point_), distance2(distance2_) {}
		bool operator< ( const ElementForQueue &b ) const {
			return distance2 < b.distance2;
		}
	};
	// KNNの結果を格納するキュー
	typedef std::priority_queue< ElementForQueue, std::vector< ElementForQueue > > ResultQueue;

private:
	std::vector< T > points;
	struct KDTreeNode {
		T			*point;//フォトンの位置を格納
		KDTreeNode	*left;
		KDTreeNode	*right;
		int			axis;//評価軸。デプスがあれば代用できる。
	};

	KDTreeNode *root;

	void delete_kdtree( KDTreeNode *node ){
		if ( node == NULL ){
			return;
		}
		delete_kdtree( node->left );
		delete_kdtree( node->right );
		delete node;
	}

	//各ノードで距離が足りなければ以降のノードをスキップしたい。
	void locate_points(	typename KDTree<T>::ResultQueue* pqueue, 
						KDTreeNode* node,//探索ノード
						typename KDTree<T>::Query &query) {
		if(node==NULL){
			return;
		}
		const int axis=node->axis;

		float delta;// 探索位置と、現在ノードのマンハッタン距離。
		switch (axis) {
			case 0: 
				delta = query.search_position.x - node->point->position.x; 
				break;
			case 1: 
				delta = query.search_position.y - node->point->position.y;
				break;
			case 2: 
				delta = query.search_position.z - node->point->position.z; 
				break;
		}

		// 対象点<->探索中心の距離が設定半径以下　かつ　対象点<->探索中心の法線方向の距離が一定以下　という条件ならその対象点格納
		const Vec dir = node->point->position - query.search_position;//方向ベクタ
		const float distance2 = dir.LengthSquared();//二乗した距離。方向ベクタの事情で出す。
		const float dt = Dot(query.normal, dir / sqrt( distance2 ) );//検索クエリの法線、方向ベクタを正規化した値の内積。
		if (distance2 < query.max_distance2					//距離の二乗が検索クエリの設定された距離の二乗よりも小さい
			&& fabs(dt) <= query.max_distance2*0.01f){		//かつ検索クエリの法線と方向ベクタを正規化した値の内積の絶対値が、
															//クエリの設定された距離の二乗よりの１％よりも小さい						

			pqueue->push(ElementForQueue(node->point, distance2));//キューに現在ノードの位置を入れる

			if(pqueue->size() > query.max_search_num){//キューのサイズが検索クエリに設定された最大値よりも大きくなったら
				pqueue->pop();//結果を捨てる。
				query.max_distance2=pqueue->top().distance2;//最大の距離にキューの先頭の距離の二乗を設定
			}
		}
		if(delta>0.0f){ // みぎ：探索位置とノードのマンハッタン距離が正であれば、値が大きい右のツリーをたどることになる。
			//#### 再帰探索 ####
			locate_points(pqueue,node->right, query);//右ノードをたどる。
			if(delta*delta < query.max_distance2) {	//現在のノード（今検索したノードからすると親ノード）のマンハッタン距離の二乗よりも、
													//検索したノードの最大の距離の二乗が大きければ、左ノードを探索する。
				//#### 再帰探索 ####
				locate_points(pqueue, node->left, query);
			}
		}else{ // ひだり
			//#### 再帰探索 ####
			locate_points(pqueue,node->left, query);
			if(delta*delta<query.max_distance2){			//現在のノード（今検索したノードからすると親ノード）のマンハッタン距離の二乗よりも、
															//検索したノードの最大の距離の二乗が大きければ、右ノードを探索する。
				//#### 再帰探索 ####
				locate_points(pqueue, node->right, query);
			}
		}

	}

	static bool kdtree_less_operator_x(const T& left, const T& right) {
		return left.position.x < right.position.x;
	}
	static bool kdtree_less_operator_y(const T& left, const T& right) {
		return left.position.y < right.position.y;
	}
	static bool kdtree_less_operator_z(const T& left, const T& right) {
		return left.position.z < right.position.z;
	}

	KDTreeNode* create_kdtree_sub(	typename std::vector<T>::iterator begin,
									typename std::vector<T>::iterator end,
									int depth) {
		if(end - begin <= 0){//エラーチェック
			return NULL;
		}
		const int axis=depth%3;
		// 中央値
		switch (axis) {
			case 0: std::sort(begin, end, kdtree_less_operator_x); break;
			case 1: std::sort(begin, end, kdtree_less_operator_y); break;
			case 2: std::sort(begin, end, kdtree_less_operator_z); break;
		}
		//const int median = (end - begin)/2;//中央値取得
		const int median = (int)(end - begin)/2;//中央値取得
		KDTreeNode* node = new KDTreeNode;
		node->axis = axis;//軸を入れておく
		node->point = &(*(begin + median));//ノードのポインタ設定。中央値を入れる。
		// 子供
		node->left = create_kdtree_sub(begin, begin + median, depth + 1);//medianは左ノードに入る。
		node->right = create_kdtree_sub(begin + median + 1, end, depth + 1);
		return node;
	}

public:
	KDTree() {
		root = NULL;
	}
	virtual ~KDTree() {
		delete_kdtree(root);
	}
	size_t Size() {
		return points.size();
	}
	void SearchKNN(typename KDTree::ResultQueue* pqueue, typename KDTree<T>::Query &query) {
		locate_points(pqueue, root, query);
	}
	void AddPoint(const T &point) {
		points.push_back(point);
	}
	void CreateKDtree() {
		root=create_kdtree_sub(points.begin(), points.end(), 0);
	}
	Vec GetPoint(uint32_t index){
		return points[index].position;
	}
	T GetPointAll(uint32_t index){
		return points[index];
	}
};

#endif//