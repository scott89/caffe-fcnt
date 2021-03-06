#include <vector>

#include "caffe/layer.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype>
void SplitLayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  count_ = bottom[0]->count();
  for (int i = 0; i < top->size(); ++i) {
    // Do not allow in-place computation in the SplitLayer.  Instead, share data
    // by reference in the forward pass, and keep separate diff allocations in
    // the backward pass.  (Technically, it should be possible to share the diff
    // blob of the first split output with the input, but this seems to cause
    // some strange effects in practice...)
    CHECK_NE((*top)[i], bottom[0]) << this->type_name() << " Layer does not "
        "allow in-place computation.";
    (*top)[i]->Reshape(bottom[0]->num(), bottom[0]->channels(),
                       bottom[0]->height(), bottom[0]->width());
    CHECK_EQ(count_, (*top)[i]->count());
  }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template<typename Dtype>
void SplitLayer<Dtype>::Reduce(vector<Blob<Dtype>*>* bottom, const vector<Blob<Dtype>*>& top) {
  // Copy select vector from top to bottom blobs
  (*bottom)[0]->reset_select();
  caffe_copy(top[0]->channels(), top[0]->cpu_select(),
  static_cast<Dtype*>((*bottom)[0]->mutable_cpu_select()));

  // Reshape bottom blobs
  const int bottom_channels = top[0]->channels();
  const int bottom_num = (*bottom)[0]->num();
  const int bottom_height = (*bottom)[0]->height();
  const int bottom_width = (*bottom)[0]->width();
  (*bottom)[0]->Reshape(bottom_num, bottom_channels, bottom_height, bottom_width);
  // Reassign size parameters
  count_ = (*bottom)[0]->count();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <typename Dtype>
void SplitLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  for (int i = 0; i < top->size(); ++i) {
    (*top)[i]->ShareData(*bottom[0]);
  }
}

template <typename Dtype>
void SplitLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, vector<Blob<Dtype>*>* bottom) {
  if (!propagate_down[0]) { return; }
  if (top.size() == 1) {
    caffe_copy(count_, top[0]->cpu_diff(), (*bottom)[0]->mutable_cpu_diff());
    return;
  }
  caffe_add(count_, top[0]->cpu_diff(), top[1]->cpu_diff(),
            (*bottom)[0]->mutable_cpu_diff());
  // Add remaining top blob diffs.
  for (int i = 2; i < top.size(); ++i) {
    const Dtype* top_diff = top[i]->cpu_diff();
    Dtype* bottom_diff = (*bottom)[0]->mutable_cpu_diff();
    caffe_axpy(count_, Dtype(1.), top_diff, bottom_diff);
  }
}


#ifdef CPU_ONLY
STUB_GPU(SplitLayer);
#endif

INSTANTIATE_CLASS(SplitLayer);

}  // namespace caffe
