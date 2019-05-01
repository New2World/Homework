# machine learning homework 3 - KMeans

import sys
import numpy as np
import matplotlib.pyplot as plt

class KMeans(object):
    def __init__(self, k):
        '''
        set number of clusters
        '''
        self.k = k

    def im2sample(self, X):
        '''
        convert image to pixel array
        '''
        if not isinstance(X, np.ndarray):
            X = np.array(X)
        self.imshape = X.shape
        return X.reshape((-1,3)).astype(np.float32)

    def sample2im(self, X):
        '''
        reconstruct image from pixel array
        '''
        return X.reshape(self.imshape).astype(np.uint8)

    def train(self, X):
        sample = self.im2sample(X)
        segmentation = None

        # initialize random clusters
        cluster = np.random.rand(self.k,3)*255
        prev_cluster = np.zeros((self.k,3))
        iteration = 0

        # until converge
        while not (cluster==prev_cluster).all():
            iteration += 1
            prev_cluster = cluster.copy()

            # calculate distance
            sample_dist = np.vstack([np.sum((sample-cluster[i])**2, axis=1) for i in range(self.k)]).T
            segmentation = np.argmin(sample_dist, axis=1)

            # calculate mean of each segmentation
            for i in range(self.k):
                subsample = sample[segmentation==i]
                if subsample.shape[0] == 0:
                    cluster[i] = (0.,0.,0.)
                else:
                    cluster[i] = np.mean(subsample, axis=0)

        # draw segmentation base on clusters
        for i in range(self.k):
            sample[segmentation==i] = cluster[i]
        X = self.sample2im(sample)
        return X, iteration


def image_segmentation(image, k):
    kmeans = KMeans(k)
    print "  k = {}".format(k)
    new_image, iters = kmeans.train(image)
    return new_image

if __name__ == "__main__":
    if len(sys.argv) > 1:
        image = plt.imread(sys.argv[1],1)
        for k in (2,3,5,10):
            new_image = image_segmentation(image, k)
            plt.imsave("image_segment_{}.jpg".format(k), new_image)
    else:
        for i in range(4):
            image_path = "udel{0}/udel{0}.jpg".format(i+1)
            print "processing "+image_path
            image = plt.imread(image_path, 1)
            for k in (2,3,5,10):
                new_image = image_segmentation(image, k)
                plt.imsave("udel{0}/udel{0}_{1}.jpg".format(i+1, k), new_image)
                if i == 3:
                    break