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
// KD-tree�N���X
//----------------------------------------------------------------
template<typename T>
class KDTree {
public:
	// k-NN search�̃N�G���i����j
	struct Query {
		float max_distance2; // �T���̍ő唼�a
		size_t max_search_num; // �ő�T���_��
		Vec search_position; // �T�����S
		Vec normal; // �T�����S�ɂ�����@��
		Query(const Vec &search_position_, const Vec &normal_, const float max_distance2_, const size_t max_search_num_) :
			max_distance2(max_distance2_), 
			normal(normal_), 
			max_search_num(max_search_num_), 
			search_position(search_position_) {}
	};
	// ���ʂ�Queue�ɏ悹�邽�߂̃f�[�^�\���B
	struct ElementForQueue {
		const T *point;
		float distance2;
		ElementForQueue(const T *point_, const float distance2_) : point(point_), distance2(distance2_) {}
		bool operator< ( const ElementForQueue &b ) const {
			return distance2 < b.distance2;
		}
	};
	// KNN�̌��ʂ��i�[����L���[
	typedef std::priority_queue< ElementForQueue, std::vector< ElementForQueue > > ResultQueue;

private:
	std::vector< T > points;
	struct KDTreeNode {
		T			*point;//�t�H�g���̈ʒu���i�[
		KDTreeNode	*left;
		KDTreeNode	*right;
		int			axis;//�]�����B�f�v�X������Α�p�ł���B
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

	//�e�m�[�h�ŋ���������Ȃ���Έȍ~�̃m�[�h���X�L�b�v�������B
	void locate_points(	typename KDTree<T>::ResultQueue* pqueue, 
						KDTreeNode* node,//�T���m�[�h
						typename KDTree<T>::Query &query) {
		if(node==NULL){
			return;
		}
		const int axis=node->axis;

		float delta;// �T���ʒu�ƁA���݃m�[�h�̃}���n�b�^�������B
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

		// �Ώۓ_<->�T�����S�̋������ݒ蔼�a�ȉ��@���@�Ώۓ_<->�T�����S�̖@�������̋��������ȉ��@�Ƃ��������Ȃ炻�̑Ώۓ_�i�[
		const Vec dir = node->point->position - query.search_position;//�����x�N�^
		const float distance2 = dir.LengthSquared();//��悵�������B�����x�N�^�̎���ŏo���B
		const float dt = Dot(query.normal, dir / sqrt( distance2 ) );//�����N�G���̖@���A�����x�N�^�𐳋K�������l�̓��ρB
		if (distance2 < query.max_distance2					//�����̓�悪�����N�G���̐ݒ肳�ꂽ�����̓�����������
			&& fabs(dt) <= query.max_distance2*0.01f){		//�������N�G���̖@���ƕ����x�N�^�𐳋K�������l�̓��ς̐�Βl���A
															//�N�G���̐ݒ肳�ꂽ�����̓����̂P������������						

			pqueue->push(ElementForQueue(node->point, distance2));//�L���[�Ɍ��݃m�[�h�̈ʒu������

			if(pqueue->size() > query.max_search_num){//�L���[�̃T�C�Y�������N�G���ɐݒ肳�ꂽ�ő�l�����傫���Ȃ�����
				pqueue->pop();//���ʂ��̂Ă�B
				query.max_distance2=pqueue->top().distance2;//�ő�̋����ɃL���[�̐擪�̋����̓���ݒ�
			}
		}
		if(delta>0.0f){ // �݂��F�T���ʒu�ƃm�[�h�̃}���n�b�^�����������ł���΁A�l���傫���E�̃c���[�����ǂ邱�ƂɂȂ�B
			//#### �ċA�T�� ####
			locate_points(pqueue,node->right, query);//�E�m�[�h�����ǂ�B
			if(delta*delta < query.max_distance2) {	//���݂̃m�[�h�i�����������m�[�h���炷��Ɛe�m�[�h�j�̃}���n�b�^�������̓������A
													//���������m�[�h�̍ő�̋����̓�悪�傫����΁A���m�[�h��T������B
				//#### �ċA�T�� ####
				locate_points(pqueue, node->left, query);
			}
		}else{ // �Ђ���
			//#### �ċA�T�� ####
			locate_points(pqueue,node->left, query);
			if(delta*delta<query.max_distance2){			//���݂̃m�[�h�i�����������m�[�h���炷��Ɛe�m�[�h�j�̃}���n�b�^�������̓������A
															//���������m�[�h�̍ő�̋����̓�悪�傫����΁A�E�m�[�h��T������B
				//#### �ċA�T�� ####
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
		if(end - begin <= 0){//�G���[�`�F�b�N
			return NULL;
		}
		const int axis=depth%3;
		// �����l
		switch (axis) {
			case 0: std::sort(begin, end, kdtree_less_operator_x); break;
			case 1: std::sort(begin, end, kdtree_less_operator_y); break;
			case 2: std::sort(begin, end, kdtree_less_operator_z); break;
		}
		//const int median = (end - begin)/2;//�����l�擾
		const int median = (int)(end - begin)/2;//�����l�擾
		KDTreeNode* node = new KDTreeNode;
		node->axis = axis;//�������Ă���
		node->point = &(*(begin + median));//�m�[�h�̃|�C���^�ݒ�B�����l������B
		// �q��
		node->left = create_kdtree_sub(begin, begin + median, depth + 1);//median�͍��m�[�h�ɓ���B
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