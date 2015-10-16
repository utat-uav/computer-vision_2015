import cv2
import numpy as np
import sys

class Segment:
	def __init__(self, MAX_CLUSTERS = None, MAX_NUM_KMEANS_ITER = None, MAX_KMEANS_EPSILON = None):
		self.MAX_CLUSTERS = MAX_CLUSTERS or 3
		self.MAX_NUM_KMEANS_ITER = MAX_NUM_KMEANS_ITER or 10
		self.MAX_KMEANS_EPSILON = MAX_KMEANS_EPSILON or 1.0
		
		self.KMEANS_TERMINATOR = (cv2.TERM_CRITERIA_MAX_ITER + cv2.TERM_CRITERIA_EPS, self.MAX_NUM_KMEANS_ITER, self.MAX_KMEANS_EPSILON)
	
	def segment(self, img_path):
		print("Converting to Lab...")
		img = cv2.imread(img_path)
		rows, cols = len(img), len(img[0])
		lab_img = cv2.cvtColor(img, cv2.COLOR_RGB2LAB)
		cv2.imwrite("labim.jpg", lab_img)
		
		print("Rows:", rows, "Cols:", cols)
		#only transfer 2nd and 3rd (A and B) channels to `abim`
		lab_channels = cv2.split(lab_img)
		abim = cv2.merge(np.delete(lab_channels, 0, 0)) #remove the L channel
		
		print("Reshaping...")
		abim_flat = np.float32(np.reshape(abim, (rows*cols, 2))) #flatten to a rows*col length array with 2 channels
		
		print("Running KMeans")
		retval, labels_flat, centers = cv2.kmeans(abim_flat, self.MAX_CLUSTERS, self.KMEANS_TERMINATOR, 2, cv2.KMEANS_RANDOM_CENTERS)
		
		print("Reshaping...")
		#transform from flat to original image dimensions by feeding in data throught he buffer row-by-row
		labels = np.reshape(labels_flat, (rows, cols)) #reshape the flat labels array to original dimensions
		
		print("Saving labels...")
		#{MAX_CLUSTERS}-color greyscale image
		cv2.imwrite("labels.jpg", labels*70)
		
		isolated_labels = []
		for i in range(self.MAX_CLUSTERS):
			#map equality function onto all elements in `labels` and push the result (a matrix the size of `labels` with only 0 or 1) onto `isolated_labels`. It's important to cast the Bool to np.int since `findContours` won't accept an array "of type=0"
			isolated_labels.append(np.vectorize(lambda x: np.int(x==i))(labels))
			cv2.imwrite("labels"+str(i)+".jpg", isolated_labels[i]*255)
		
		##CONVEXHULL
		
		#first, generate a list of contours, each a list of tuples (x,y)
		shape_contours, hierarchy = cv2.findContours(isolated_labels[2].astype(np.uint8), cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
		#Find the most complex shape: the contour with the most points
		most_complex_shape = reduce(lambda x, y: x if len(x)>len(y) else y, shape_contours)
		#Run `convexHull`
		outHull = cv2.convexHull(most_complex_shape)
		
		#create a blank, black image and draw the simplified shape onto it
		hull_img = np.zeros((rows, cols, 3))
		cv2.fillConvexPoly(hull_img, outHull, [255, 255, 255])
		
		cv2.imwrite("hull.jpg", hull_img)

if __name__ == "__main__":
	Segment().segment(sys.argv[1])