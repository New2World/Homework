import sys, math
import numpy as np
import matplotlib.pyplot as plt

fn = lambda x, u, d: 1/math.sqrt(2*math.pi)/d*np.exp(-(x-u)*(x-u)/2/np.sqrt(d))

def read_data(file_name):
    data = []
    with open(file_name,"r") as fd:
        for data_line in fd:
            data.append(float(data_line))
    return np.sort(np.array(data))

def plot_data(data, u, d):
    Xmin = int(data.min())
    Xmax = int(data.max())
    color = ['red','green','blue','black','yellow']
    plt.scatter(data,[0]*data.shape[0],marker='x',c='r')
    X = [x/10. for x in range(Xmin*10,(Xmax+1)*10)]
    for i in range(len(u)):
        plt.plot(X, fn(X,u[i],d[i]),c=color[i%5])
    plt.show()

class EM(object):
    def expectation(self, X, u, d, a, k):
        weight_prob = np.array([fn(xi, u, d)*a for xi in X])
        w = weight_prob/np.sum(weight_prob, axis=1)[:, np.newaxis]
        return w
    
    def maximization(self, X, w, k):
        n = np.sum(w,axis=0)
        a = n/self.n_samples
        u = (X[np.newaxis,:].dot(w)/n).squeeze()
        d = np.array([w[:,i].dot((X-u[i])*(X-u[i])).T/n[i] for i in range(k)])
        return u, np.sqrt(d), a
    
    def mle(self, X):
        u = X.mean()
        d = X.std()
        return [u], [d]

    def train(self, X, k, episode=50):
        # if only one Gaussian, use MLE to estimate
        if k == 1:
            u, d = self.mle(X)
            return u, d, None, None

        # initialization
        w = None
        a = np.random.rand(k)
        u = np.random.uniform(X.min(), X.max(), k)
        d = np.random.rand(k)
        self.n_samples = X.shape[0]
        prev = None
        diff = 1

        # EM algorithm
        iters = 0
        while diff > 1e-3:
            w = self.expectation(X, u, d, a, k)
            prev = u
            u, d, a = self.maximization(X, w, k)
            diff = np.sum(np.abs(u-prev))
            iters += 1
            if iters > 100:
                break
        
        return u, d, w, a

k = 3
if len(sys.argv) > 1:
    k = int(sys.argv[1])
em = EM()
X = read_data("em_data.txt")
u, d, w, a = em.train(X, k=k, episode=10)
print "mu =", u
print "std =", d
plot_data(X, u, d)


# k = 1
# mu = [15.481698203115867]
# std = [8.216747428495669]
#
# k = 3
# mu = [15.44916079 25.48665443  5.5092794 ]
# std = [0.98342053 0.99904786 1.01501609]
#
# k = 5
# mu = [25.48665443 15.47529217 15.44221599  5.5092794  15.47006884]
# std = [0.99904786 0.98192623 0.98370894 1.01501609 0.98220679]