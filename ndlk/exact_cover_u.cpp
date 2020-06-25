/* exact_cover_u.cpp */

class Exact_cover_u {
	typedef unsigned short elem_t;

  unsigned order; // size of the LK

  std::vector<elem_t> transversals;
  size_t num_trans;
  elem_t* add_trans();
  const elem_t* get_trans(size_t i) const;

  void search_trans(const Square& lk)
  {
    order= lk.width()
    transversals.resize(0);
    num_trans=0;
    init_trans(lk);
    search_trans();
  }

  bool is_ortogon()
  {
    init_disjoint();
    return search_disjoint(0);
  }

  void search_mates(std::list<Square>& mates)
  {
    init_disjoint();
    search_disjoint(&mates);
  }
};
