/**
 * @file flavor-wheel-view.js
 * @brief Draws the flavor-wheel frequency analysis as an SVG sunburst.
 *
 * Inner ring: the nine categories; middle: groups; outer: descriptors. Every
 * outermost segment gets the same angle. A segment's opacity follows the share
 * of regions that placed notes in it; segments named by fewer than half of the
 * regions (contested) are hatched and outlined so the split stands out.
 */

const SVG_NS = 'http://www.w3.org/2000/svg';

/** Ring radii in viewBox units (the wheel is drawn in a 300 x 300 box). */
const RINGS = Object.freeze({ 1: [30, 74], 2: [74, 110], 3: [110, 146] });
const CENTER = 150;

/** Inner-ring hues, after the SCA / WCR poster. */
const CATEGORY_COLORS = Object.freeze({
  Floral: '#da0d68',
  Fruity: '#da1d23',
  'Sour/Fermented': '#e0a80f',
  'Green/Vegetative': '#187a2f',
  Other: '#0a93a5',
  Roasted: '#c94930',
  Spices: '#ad213e',
  'Nutty/Cocoa': '#a87b64',
  Sweet: '#e65832',
});

/** One segment with its computed angles. */
class WheelArc {
  constructor({ segment, category, start, end }) {
    this.segment = segment;
    this.category = category;
    this.start = start;
    this.end = end;
    Object.freeze(this);
  }
}

export class FlavorWheelView {
  static #REGION_LABELS = Object.freeze({ KO: 'KO', JA: 'JA', 'ZH-TW': 'TW', EN: 'EN', NO: 'NO', DE: 'DE' });

  #document;
  #dom;
  #i18n;

  constructor(documentRef, dom, translator) {
    this.#document = documentRef;
    this.#dom = dom;
    this.#i18n = translator;
  }

  /** Returns the wheel block, or null when the profile has no wheel data. */
  build(segments) {
    if (!segments.length) return null;
    const block = this.#dom.element('div', 'wheel');
    block.append(this.#dom.element('p', 'section', this.#i18n.t('wheel.title')));
    if (!segments.some((segment) => segment.regionCount > 0)) {
      block.append(this.#dom.element('p', 'muted small', this.#i18n.t('wheel.empty')));
      return block;
    }
    const arcs = this.#layout(segments);
    block.append(this.#drawSvg(arcs, this.#regionTotal(segments)), this.#legend());
    const split = this.#splitList(segments);
    if (split) block.append(split);
    block.append(this.#dom.element('p', 'wheel-credit', this.#i18n.t('wheel.credit')));
    return block;
  }

  #layout(segments) {
    const children = new Map();
    for (const segment of segments) {
      if (!children.has(segment.parentPath)) children.set(segment.parentPath, []);
      children.get(segment.parentPath).push(segment);
    }
    const leafCount = (segment) => {
      const kids = children.get(segment.path) ?? [];
      return kids.length ? kids.reduce((sum, kid) => sum + leafCount(kid), 0) : 1;
    };
    const roots = children.get('') ?? [];
    const total = roots.reduce((sum, root) => sum + leafCount(root), 0);
    const arcs = [];
    const place = (segment, start, category) => {
      const span = (leafCount(segment) / total) * Math.PI * 2;
      arcs.push(new WheelArc({ segment, category, start, end: start + span }));
      let cursor = start;
      for (const kid of children.get(segment.path) ?? []) cursor += place(kid, cursor, category);
      return span;
    };
    let cursor = -Math.PI / 2;
    for (const root of roots) cursor += place(root, cursor, root.name);
    return arcs;
  }

  #drawSvg(arcs, regionTotal) {
    const svg = this.#svg('svg', { viewBox: '0 0 300 300', class: 'wheel-svg', role: 'img' });
    svg.setAttribute('aria-label', this.#i18n.t('wheel.title'));
    svg.append(this.#hatchPattern());
    for (const arc of arcs) svg.append(this.#segmentNode(arc, regionTotal));
    for (const arc of arcs) {
      const label = this.#labelNode(arc);
      if (label) svg.append(label);
    }
    const hub = this.#svg('text', { x: CENTER, y: CENTER + 4, class: 'wheel-hub' });
    hub.textContent = `${regionTotal}/6`;
    svg.append(hub);
    return svg;
  }

  #segmentNode(arc, regionTotal) {
    const { segment } = arc;
    const [inner, outer] = RINGS[segment.tier] ?? RINGS[3];
    const group = this.#svg('g', { class: 'wheel-seg' });
    const d = this.#arcPath(inner, outer, arc.start, arc.end);
    const hit = segment.regionCount > 0;
    const opacity = hit ? 0.35 + 0.65 * Math.min(1, segment.consensusScore) : 0.1;
    group.append(this.#svg('path', {
      d, fill: CATEGORY_COLORS[arc.category] ?? '#888', 'fill-opacity': opacity.toFixed(2), class: 'wheel-fill',
    }));
    if (segment.contested) {
      group.append(this.#svg('path', { d, fill: 'url(#cupano-hatch)', class: 'wheel-contested' }));
    }
    const title = this.#svg('title', {});
    title.textContent = hit
      ? this.#i18n.t('wheel.tooltip', {
        path: this.#displayPath(segment, arc.category),
        count: segment.regionCount,
        total: regionTotal,
        regions: this.#regionList(segment.sourceRegions),
      })
      : this.#displayPath(segment, arc.category);
    group.append(title);
    return group;
  }

  #labelNode(arc) {
    const { segment } = arc;
    const span = arc.end - arc.start;
    // Category names always; groups/descriptors only where a region placed a note.
    if (segment.tier !== 1 && segment.regionCount === 0) return null;
    if (span < 0.05) return null;
    const [inner, outer] = RINGS[segment.tier] ?? RINGS[3];
    const middle = (arc.start + arc.end) / 2;
    const radius = (inner + outer) / 2;
    const x = CENTER + radius * Math.cos(middle);
    const y = CENTER + radius * Math.sin(middle);
    let degrees = (middle * 180) / Math.PI;
    // Keep text upright on the left half of the wheel.
    if (Math.cos(middle) < 0) degrees += 180;
    const text = this.#svg('text', {
      x: x.toFixed(1), y: y.toFixed(1),
      transform: `rotate(${degrees.toFixed(1)} ${x.toFixed(1)} ${y.toFixed(1)})`,
      class: `wheel-label tier-${segment.tier}${segment.regionCount ? ' hit' : ''}`,
    });
    const name = segment.tier === 1 ? this.#categoryName(segment.name) : segment.name;
    text.textContent = this.#fit(name, segment.tier, outer - inner);
    return text;
  }

  #legend() {
    const legend = this.#dom.element('div', 'wheel-legend');
    const agreed = this.#dom.element('span', 'legend-item');
    agreed.append(this.#dom.element('span', 'swatch agreed'), this.#dom.element('span', '', this.#i18n.t('wheel.agreed')));
    const split = this.#dom.element('span', 'legend-item');
    split.append(this.#dom.element('span', 'swatch split'), this.#dom.element('span', '', this.#i18n.t('wheel.split')));
    legend.append(agreed, split);
    return legend;
  }

  /** Lists the most specific contested segments with the regions that named them. */
  #splitList(segments) {
    const contested = segments.filter((segment) => segment.contested);
    const deepest = contested.filter((segment) =>
      !contested.some((other) => other.parentPath === segment.path));
    if (!deepest.length) return null;
    const box = this.#dom.element('div', 'split-notes');
    box.append(this.#dom.element('p', 'section', this.#i18n.t('wheel.splitNotes')));
    const list = this.#dom.element('ul');
    for (const segment of deepest) {
      const item = this.#dom.element('li');
      const category = segment.path.split(' > ')[0];
      const dot = this.#dom.element('span', 'dot-cat');
      dot.style.background = CATEGORY_COLORS[category] ?? '#888';
      item.append(dot, this.#dom.element('b', '', segment.tier === 1 ? this.#categoryName(segment.name) : segment.name));
      item.append(this.#dom.element('span', 'muted', ` · ${this.#i18n.t('wheel.onlyRegions', {
        regions: this.#regionList(segment.sourceRegions),
      })}`));
      list.append(item);
    }
    box.append(list);
    return box;
  }

  #hatchPattern() {
    const defs = this.#svg('defs', {});
    const pattern = this.#svg('pattern', {
      id: 'cupano-hatch', width: 5, height: 5, patternUnits: 'userSpaceOnUse', patternTransform: 'rotate(45)',
    });
    pattern.append(this.#svg('rect', { width: 5, height: 5, fill: 'transparent' }));
    pattern.append(this.#svg('line', { x1: 0, y1: 0, x2: 0, y2: 5, class: 'hatch-line' }));
    defs.append(pattern);
    return defs;
  }

  #arcPath(inner, outer, start, end) {
    const point = (radius, angle) =>
      `${(CENTER + radius * Math.cos(angle)).toFixed(2)} ${(CENTER + radius * Math.sin(angle)).toFixed(2)}`;
    const large = end - start > Math.PI ? 1 : 0;
    return [
      `M ${point(outer, start)}`,
      `A ${outer} ${outer} 0 ${large} 1 ${point(outer, end)}`,
      `L ${point(inner, end)}`,
      `A ${inner} ${inner} 0 ${large} 0 ${point(inner, start)}`,
      'Z',
    ].join(' ');
  }

  #fit(name, tier, ringWidth) {
    // Rough character budget for a radial label in this ring.
    const budget = Math.floor(ringWidth / (tier === 1 ? 4.4 : 3.6));
    return name.length > budget ? `${name.slice(0, Math.max(1, budget - 1))}…` : name;
  }

  #displayPath(segment, category) {
    const parts = segment.path.split(' > ');
    parts[0] = this.#categoryName(category);
    return parts.join(' › ');
  }

  #categoryName(name) {
    return this.#i18n.t(`wheel.cat.${name}`);
  }

  #regionList(codes) {
    return codes.map((code) => FlavorWheelView.#REGION_LABELS[code] ?? code).join(', ');
  }

  #regionTotal(segments) {
    // consensusScore = regionCount / regions that reported notes.
    const hit = segments.find((segment) => segment.regionCount > 0 && segment.consensusScore > 0);
    return hit ? Math.round(hit.regionCount / hit.consensusScore) : 0;
  }

  #svg(tag, attributes) {
    const node = this.#document.createElementNS(SVG_NS, tag);
    for (const [name, value] of Object.entries(attributes)) node.setAttribute(name, String(value));
    return node;
  }
}
