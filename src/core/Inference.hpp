#ifndef RGM_INFERENCE_HPP_
#define RGM_INFERENCE_HPP_

#include "AOGrammar.hpp"
#include "ParseTree.hpp"

namespace RGM
{
/// Parsing with AOGrammar
class Inference
{
public:
    // control detection/parsing
    struct Param {
        Param();

        Scalar thresh_;
        bool   useNMS_;
        Scalar nmsOverlap_;
        bool   nmsDividedByUnion_;
        bool   createSample_; // if true get the feature in the feature pyramid
        bool   useOverlapLoss_;
        bool   createRootSample2x_;
        bool   computeTNodeScores_;
    };

    typedef std::map<boost::uuids::uuid, std::vector<Matrix > >    Maps;
    typedef std::map<boost::uuids::uuid, std::vector<bool > >      Status;
    typedef std::map<boost::uuids::uuid, std::vector<MatrixXi> >   argMaps;

    /// Type of a detection
    typedef Detection_<Scalar> Detection;

    /// constructor
    explicit Inference(AOGrammar & g, Param & p);

    /// Computes the detection results
    void runDetection(const Scalar thresh, cv::Mat img, Scalar & maxDetNum,
                      std::vector<ParseTree> & pt, bool usePCA=false);

    void runDetection(const Scalar thresh, const FeaturePyramid & pyramid, Scalar & maxDetNum,
                      std::vector<ParseTree> & pt, bool usePCA=false);

    /// Comuptes the top 1 detection result for both original model and pca project one
    /// in learning decision policy
    void runDetection(cv::Mat img, ParseTree &pt, ParseTree &pcaPt);

    /// Computes the detection results with extended NMS used (e.g., for the car AOG in our ECCV2014 paper)
    void runDetectionExt(const Scalar thresh, cv::Mat img, Scalar & maxDetNum,
                      std::vector<ParseTree> & pt, std::vector<Detection> & alldets, bool usePCA=false);

    void runDetectionExt(const Scalar thresh, const FeaturePyramid & pyramid, Scalar & maxDetNum,
                      std::vector<ParseTree> & pt, std::vector<Detection> & alldets, bool usePCA=false);

    /// Computes the score maps using DP algorithm
    bool runDP(const FeaturePyramid & pyramid, bool usePCA=false);

    /// Runs parsing
    void runParsing(const Scalar thresh, const FeaturePyramid & pyramid,
                    Scalar & maxDetNum, std::vector<ParseTree> & pt, bool usePCA=false);

    void runParsingExt(const Scalar thresh, const FeaturePyramid & pyramid,
                    Scalar & maxDetNum, std::vector<ParseTree> & pt, std::vector<Detection> & alldets, bool usePCA=false);

    /// Computes a parse tree
    void parse(const FeaturePyramid & pyramid, Detection & cand, ParseTree & pt,
               bool getLoss=false, bool usePCA=false);   

    /// Computes overlap maps
    /// @param[in] overlapMaps It will return overlap maps for
    ///  each level in the score maps, each box in @p bboxes and each object AND-node
    /// @param[out] the valid state of levels
    std::vector<bool> computeOverlapMaps(const std::vector<Rectangle2i> & bboxes, const FeaturePyramid & pyr,
                                         std::vector<std::vector<std::vector<Matrix> > > & overlapMaps, Scalar overlapThr);

    /// Copies score maps of a node
    void copyScoreMaps(const Node * n);

    /// Recovers score maps of a node
    void recoverScoreMaps(const Node * n);

    /// Applies output inhibition of a given bounding box
    /// Inhibit all detection window locations that do not yield sufficient overlap with the
    /// given bounding box by setting the score = -inf
    void inhibitOutput(int idxBox, std::vector<std::vector<std::vector<Matrix> > > & overlapMap,
                       Scalar overlapThr, bool needCopy);

    /// Applies loss adjustment
    void applyLossAdjustment(int idxBox, int nbBoxes, std::vector<std::vector<std::vector<Matrix> > > & overlapMap,
                             Scalar fgOverlap, Scalar bgOverlap, bool needCopy);

private:
    /// Computes filter responses of T-nodes, i.e., alpha-processes
    bool computeAlphaProcesses(const FeaturePyramid & pyramid);

#if RGM_USE_PCA_DIM
    bool computePCAAlphaProcesses(const FeaturePyramid & pyramid);
#endif

    /// Computes the scale prior feature
    void computeScalePriorFeature(int nbLevels);

    /// Applies the compositional rule or the deformation rule for an AND-node
    bool computeANDNode(Node * node, int padx, int pady);

    /// Bounded DT
    void DT2D(Matrix & scoreMap, Deformation::Param & w, int shift, MatrixXi & Ix, MatrixXi & Iy);
    void DT1D(const Scalar *vals, Scalar *out_vals, int *I, int step, int shift, int n, Scalar a, Scalar b);

    /// Applies the switching rule for an OR-node
    bool computeORNode(Node * node);

    /// Parses an OR-node
    bool parseORNode(int head, std::vector<Node *> & gBFS, std::vector<int> & ptBFS,
                     const FeaturePyramid & pyramid, ParseTree & pt, bool getLoss);

    /// Parses an AND-node
    bool parseANDNode(int head, std::vector<Node *> & gBFS, std::vector<int> & ptBFS,
                      const FeaturePyramid & pyramid, ParseTree & pt);

    /// Parses an T-node
    bool parseTNode(int idx, std::vector<Node *> & gBFS, std::vector<int> & ptBFS,
                    const FeaturePyramid & pyramid, ParseTree & pt, bool usePCA=false);

    /// Returns score maps of a node
    const std::vector<Matrix >& scoreMaps(const Node * n) ;
    std::vector<Matrix >& getScoreMaps(const Node * n);

    /// Returns copied score maps of a node
    const std::vector<Matrix >& scoreMapCopies(const Node * n) ;
    std::vector<Matrix >& getScoreMapCopies(const Node * n);

    /// Returns status of score maps of a node
    const std::vector<bool>& scoreMapStatus(const Node * n) ;
    std::vector<bool>& getScoreMapStatus(const Node * n);

    /// Sets the status of score maps of a node
    void setScoreMapStatus(const Node* n, int l);

    /// Set score maps to a node
    void setScoreMaps(const Node * n, int nbLevels, std::vector<Matrix> & s, const std::vector<bool> & validLevels);

    /// Returns the deformation maps
    std::vector<MatrixXi>& getDeformationX(const Node * n);
    std::vector<MatrixXi>& getDeformationY(const Node * n);

    /// Returns loss maps of a node
    const std::vector<Matrix >& lossMaps(const Node * n) ;
    std::vector<Matrix >& getLossMaps(const Node * n);

    /// Release memory
    void release();

private:
    AOGrammar* grammar_;

    Param*  param_;

    Matrix  scalepriorFeatures_; // a 3 * nbLevels matrix

    Maps    scoreMaps_; // for nodes in an AOG, for each level in the feature pyramid
    Maps    scoreMapCopies_;
    Status  scoreMapStatus_;
    argMaps deformationX_;
    argMaps deformationY_;
    Maps    lossMaps_;

    DEFINE_RGM_LOGGER;

}; // class Inference


} // namespace RGM

#endif // RGM_INFERENCE_HPP_
