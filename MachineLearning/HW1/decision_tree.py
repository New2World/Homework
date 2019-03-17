# machine learning homework 1 - decision tree
# author: Ritchie

import os, sys
import math, random
import numpy as np
import pandas as pd

def read_data(file_name, extra_info):
    try:
        csv_data = pd.read_csv(file_name, header=0, delimiter=',')
    except IOError:
        print "please check your path to the data file (*.csv)"
    features = csv_data.values[:,:-1]
    labels = csv_data.values[:,-1]
    feature_name = csv_data.columns[:-1]
    return features, labels, feature_name

class tree_node(object):
    def __init__(self, feat_id, label):
        self.id = -1
        self.feat_id = feat_id
        self.label = label
        self.lnode = None
        self.rnode = None

class DecisionTree(object):
    def __init__(self, heuristic='entropy'):
        self.root = None
        self.node_count = 0
        self.n_features = 0
        self.feature_name = None
        self.__heuristic = self.__info_gain if heuristic=='entropy' else self.__impurity

    def __data_check(self, x):
        if not isinstance(x, np.ndarray):
            x = np.array(x)
        return x

    def __entropy(self, y):
        if len(y) == 0:
            return 0
        prob_true = 1.*np.sum(y==1)/len(y)
        prob_false = 1.*np.sum(y==0)/len(y)
        ent_true = 0 if prob_true == 0 else prob_true*math.log(prob_true,2)
        ent_false = 0 if prob_false == 0 else prob_false*math.log(prob_false,2)
        return -(ent_true+ent_false)

    def __cond_ent(self, feat_id, x, y):
        if len(y) == 0:
            return 0
        feat_true = y[x[:,feat_id]==1]
        feat_false = y[x[:,feat_id]==0]
        ent_true = self.__entropy(feat_true)
        ent_false = self.__entropy(feat_false)
        prob_true = 1.*len(feat_true)/len(y)
        prob_false = 1.*len(feat_false)/len(y)
        return prob_true*ent_true+prob_false*ent_false

    def __info_gain(self, feat_id, x, y):
        info_gain = self.__entropy(y)-self.__cond_ent(feat_id, x, y)
        return info_gain

    def __variance_impurity(self, y):
        if len(y) == 0:
            return 0
        prob_true = 1.*np.sum(y==1)/len(y)
        prob_false = 1.*np.sum(y==0)/len(y)
        return prob_true*prob_false

    def __cond_vi(self, feat_id, x, y):
        if len(y) == 0:
            return 0
        feat_true = y[x[:,feat_id]==1]
        feat_false = y[x[:,feat_id]==0]
        vi_true = self.__variance_impurity(feat_true)
        vi_false = self.__variance_impurity(feat_false)
        prob_true = 1.*len(feat_true)/len(y)
        prob_false = 1.*len(feat_false)/len(y)
        return prob_true*vi_true+prob_false*vi_false

    def __impurity(self, feat_id, x, y):
        purity_gain = self.__variance_impurity(y)-self.__cond_vi(feat_id, x, y)
        return purity_gain

    def __build_tree(self, x, y):
        info_gain = [self.__heuristic(i, x, y) for i in range(self.n_features)]
        feat_id = np.argmax(info_gain)
        if info_gain[feat_id] <= 0:
            root = tree_node(-1, int(round(1.*np.sum(y)/len(y))))
            return root
        x_true, y_true = x[x[:,feat_id]==1], y[x[:,feat_id]==1]
        x_false, y_false = x[x[:,feat_id]==0], y[x[:,feat_id]==0]
        root = tree_node(feat_id, int(round(1.*np.sum(y)/len(y))))
        root.lnode = self.__build_tree(x_true, y_true)
        root.id = self.node_count
        self.node_count += 1
        root.rnode = self.__build_tree(x_false, y_false)
        return root

    def __find_id(self, node, id):
        if node == None:
            return None
        if node.id == id:
            return node
        if node.id > id:
            return self.__find_id(node.lnode, id)
        return self.__find_id(node.rnode, id)

    def __get_node_by_id(self, id):
        return self.__find_id(self.root, id)

    def train(self, X, y, feature_name):
        self.root = None
        X = self.__data_check(X)
        y = self.__data_check(y)
        self.n_features = X.shape[1]
        self.feature_name = feature_name
        self.root = self.__build_tree(X, y)
        return self.root

    def predict(self, X):
        X = self.__data_check(X)
        y = []
        for sample in X:
            ptr = self.root
            while ptr.feat_id >= 0:
                if sample[ptr.feat_id] == 1:
                    ptr = ptr.lnode
                else:
                    ptr = ptr.rnode
            y.append(ptr.label)
        return np.array(y)

    def evaluate(self, X, y):
        X = self.__data_check(X)
        y = self.__data_check(y)
        _y = self.predict(X)
        score = 1-1.*len(np.nonzero(_y-y)[0])/len(y)
        return score

    def __describe_tree(self, root, outp):
        description = ""
        if root.feat_id < 0:
            description += " : {}".format(root.label)
            return description
        description += "\n"+outp+"{} = 0".format(self.feature_name[root.feat_id])
        description += self.__describe_tree(root.rnode, outp+"| ")
        description += "\n"+outp+"{} = 1".format(self.feature_name[root.feat_id])
        description += self.__describe_tree(root.lnode, outp+"| ")
        return description

    def plot_tree(self):
        print self.__describe_tree(self.root, "")

    def __copy_tree(self, root):
        if root == None:
            return
        node = tree_node(root.feat_id, root.label)
        node.id = root.id
        node.lnode = self.__copy_tree(root.lnode)
        node.rnode = self.__copy_tree(root.rnode)
        return node

    def post_pruning(self, X, y, L, K):
        '''
        ugly
        - does not update node id
        '''
        orig_tree = self.root
        best_tree = None
        best_acc = self.evaluate(X, y)
        for _ in range(L):
            self.root = self.__copy_tree(orig_tree)
            M = random.randint(1, K)
            for _ in range(M):
                P = random.randint(0, self.node_count-1)
                node = self.__get_node_by_id(P)
                node.feat_id = -1
            acc = self.evaluate(X, y)
            if acc > best_acc:
                best_tree = self.__copy_tree(self.root)
                best_acc = acc
        if best_tree == None:
            self.root = orig_tree
        else:
            self.root = best_tree
        return self.root


L = int(sys.argv[1])
K = int(sys.argv[2])
training_set = sys.argv[3]
validation_set = sys.argv[4]
test_set = sys.argv[5]
if_print = True if sys.argv[6].lower() == "yes" else False

training_samples, training_labels, feature_name = read_data(training_set, "training_set.csv")
validation_samples, validation_labels,_ = read_data(validation_set, "validation_set.csv")
test_samples, test_labels,_ = read_data(test_set, "test_set.csv")

dt_1 = DecisionTree(heuristic='entropy')
dt_1.train(training_samples, training_labels, feature_name)
print "using entropy:"
print "  test accuracy: {}".format(dt_1.evaluate(test_samples,test_labels))
if if_print:
    dt_1.plot_tree()
    print
dt_2 = DecisionTree(heuristic='impurity')
dt_2.train(training_samples, training_labels, feature_name)
print "using impurity:"
print "  test accuracy: {}".format(dt_2.evaluate(test_samples,test_labels))
if if_print:
    dt_2.plot_tree()
print
print "entropy pruning:"
dt_1.post_pruning(validation_samples, validation_labels, L, K)
print "  test accuracy (after pruning): {}".format(dt_1.evaluate(test_samples,test_labels))
print "impurity pruning:"
dt_2.post_pruning(validation_samples, validation_labels, L, K)
print "  test accuracy (after pruning): {}".format(dt_2.evaluate(test_samples,test_labels))
